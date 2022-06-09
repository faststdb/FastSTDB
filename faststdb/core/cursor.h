/**
 * \file cursor.h
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef FASTSTDB_CORE_CURSOR_H_
#define FASTSTDB_CORE_CURSOR_H_

#include <memory>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <functional>

#include "faststdb/query/internal_cursor.h"
#include "faststdb/query/external_cursor.h"

namespace faststdb {

std::ostream& operator<<(std::ostream& st, Sample res);

//! Combined cursor interface
struct Cursor : InternalCursor, ExternalCursor {};

/**
 * @brief The ConcurrentCursor struct
 * Implements cursor interface. Starts computation in parallel thread.
 * All communication is done through message queue.
 */
struct ConcurrentCursor : Cursor {
  struct BufferT {
    std::vector<u8> buf;
    size_t rdpos;
    size_t wrpos;
  };

  std::thread thread_;
  mutable std::mutex  mutex_;
  std::condition_variable cond_;
  std::atomic_bool done_;
  std::deque<std::shared_ptr<BufferT>> queue_;
  common::Status error_code_;
  std::string error_message_;

  ConcurrentCursor();

  // External cursor implementation

  virtual u32 read(void* buffer, u32 buffer_size);

  virtual bool is_done() const;

  virtual bool is_error(common::Status* out_error_code_or_null = nullptr) const;
  virtual bool is_error(const char** error_message, common::Status* out_error_code_or_null) const;

  virtual void close();

  // Internal cursor implementation

  void set_error(common::Status error_code);
  void set_error(common::Status error_code, const char* error_message);

  bool put(Sample const& result);

  void complete();

  template <class Fn_1arg_caller> void start(Fn_1arg_caller const& fn) {
    thread_ = std::thread(fn);
  }

  template <class Fn_1arg> static std::unique_ptr<ExternalCursor> make(Fn_1arg const& fn) {
    std::unique_ptr<ConcurrentCursor> cursor(new ConcurrentCursor());
    cursor->start(fn);
    return std::move(cursor);
  }

  template <class Fn_2arg, class Tobj, class T2nd>
  static std::unique_ptr<ExternalCursor> make(Fn_2arg const& fn, Tobj obj, T2nd const& arg2) {
    std::unique_ptr<ConcurrentCursor> cursor(new ConcurrentCursor());
    cursor->start(std::bind(fn, obj, cursor.get(), arg2));
    return std::move(cursor);
  }

  template <class Fn_3arg, class Tobj, class T2nd, class T3rd>
  static std::unique_ptr<ExternalCursor> make(Fn_3arg const& fn, Tobj obj, T2nd const& arg2,
                                              T3rd const& arg3) {
    std::unique_ptr<ConcurrentCursor> cursor(new ConcurrentCursor());
    cursor->start(
        std::bind(fn, obj, cursor.get(), arg2, arg3));
    return std::move(cursor);
  }

  template <class Fn_4arg, class Tobj, class T2nd, class T3rd, class T4th>
  static std::unique_ptr<ExternalCursor> make(Fn_4arg const& fn, Tobj obj, T2nd const& arg2,
                                                T3rd const& arg3, T4th const& arg4) {
    std::unique_ptr<ConcurrentCursor> cursor(new ConcurrentCursor());
    cursor->start(
        std::bind(fn, obj, cursor.get(), arg2, arg3, arg4));
    return std::move(cursor);
  }
};

}  // namespace faststdb

#endif  // FASTSTDB_CORE_CURSOR_H_

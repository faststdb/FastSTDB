/*
 * \file blocking_queue.h
 * \brief The blocking queue
 */
#ifndef FASTSTDB_COMMON_BLOCKING_QUEUE_H_
#define FASTSTDB_COMMON_BLOCKING_QUEUE_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <utility>
#include <vector>

namespace faststdb {
namespace common {

template <typename T>
class BlockingQueue {
 public:
  BlockingQueue() { exit_.store(false); }

  // Push an element into the queue, the function is based on move semantics.
  void Push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.push(item);
    empty_condition_.notify_one();
  }

  // Pop an element from the queue, if the queue is empty, thread call pop would
  // be blocked.
  bool Pop(T& result) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (buffer_.empty() && !exit_) {
      empty_condition_.wait(lock);
    }
    if (buffer_.empty()) return false;
    result = std::move(buffer_.front());
    buffer_.pop();
    return true;
  }

  // Pop all elemnts from the queue, if the queue is empty, thread call pop
  // would be blocked.
  bool Pop(std::vector<T>& result) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (buffer_.empty() && !exit_) {
      empty_condition_.wait(lock);
    }
    if (buffer_.empty()) return false;
    result.clear();
    while (!buffer_.empty()) {
      result.emplace_back(std::move(buffer_.front()));
      buffer_.pop();
    }
    return true;
  }

  // Get the number of elements in the queue
  int Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(buffer_.size());
  }

  // Whether queue is empty or not
  bool Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.empty();
  }

  // Exit queue, awake all threads blocked by the queue
  void Exit() {
    std::lock_guard<std::mutex> lock(mutex_);
    exit_.store(true);
    empty_condition_.notify_all();
  }

  // Whether alive
  bool Alive() {
    std::lock_guard<std::mutex> lock(mutex_);
    return exit_ == false;
  }

 protected:
  std::queue<T> buffer_;
  mutable std::mutex mutex_;
  std::condition_variable empty_condition_;
  std::atomic_bool exit_;

  BlockingQueue(const BlockingQueue&);
  void operator=(const BlockingQueue&);
};

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_BLOCKING_QUEUE_H_

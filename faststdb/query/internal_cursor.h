/**
 * \file internal_cursor.h
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
#ifndef FASTSTDB_QUERY_INTERNAL_CURSOR_H_
#define FASTSTDB_QUERY_INTERNAL_CURSOR_H_

#include "faststdb/common/basic.h"

namespace faststdb {

/** Interface used by different search procedures
 *  in FastSTDB. Must be used only inside library.
 */
struct InternalCursor {
    //! Send offset to caller
    virtual bool put(Sample const& offset) = 0;
    virtual void complete() = 0;
    //! Set error and stop execution
    virtual void set_error(common::Status error_code) = 0;
    virtual void set_error(common::Status error_code, const char* error_message) = 0;
};

}  // namespace faststdb

#endif  // FASTSTDB_QUERY_INTERNAL_CURSOR_H_

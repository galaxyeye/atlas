/*
 * result.h
 *
 *  Created on: Aug 8, 2013
 *      Author: vincent
 */

/*    Copyright 2011 ~ 2013 Vincent Zhang, ivincent.zhang@gmail.com
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef ATLAS_RFC_RESULT_H_
#define ATLAS_RFC_RESULT_H_

#include <string>
#include <memory>

namespace atlas {
  namespace rpc {

    struct __rpc_result {
      __rpc_result(const std::string& data = "", int ec = 0) : data(data), ec(ec) {}

      __rpc_result(std::string&& data, int ec) : data(data), ec(ec) { }

      __rpc_result(const __rpc_result& other) : data(other.data), ec(other.ec) {}

      std::string data;
      int ec;
    };

    // the result of the function call to the remote side
    // every result brings the result data and an error code, 0 means no error
    // null result means we no response to the remote caller
    class rpc_result {
    public:

      // null result must be the final result
      rpc_result(std::nullptr_t) {}

      rpc_result(const std::string& data = "", int ec = 0) : _impl(new __rpc_result(data, ec)) { }

      rpc_result(std::string&& data, int ec = 0) : _impl(new __rpc_result(data, ec)) { }

      rpc_result(const rpc_result& other) {
        if (other._impl) _impl.reset(new __rpc_result(*other._impl));
      }

      rpc_result(rpc_result&& r) : _impl(std::move(r._impl)) {}

      rpc_result& operator=(const rpc_result& other) {
        if (std::addressof(other) == this) return *this;

        if (other._impl) _impl.reset(new __rpc_result(*other._impl));
        return *this;
      }

      rpc_result& operator=(rpc_result&& other) {
        if (std::addressof(other) == this) return *this;

        _impl = other._impl;
        return *this;
      }

    public:

      void reset(const std::string& data = "", int ec = 0) {
        _impl.reset(new __rpc_result(data, ec));
      }

      operator bool() const { return _impl.operator bool(); }

    public:

      const std::string& data() const { return _impl->data; }
      int err() const { return _impl->ec; }

    private:

      std::shared_ptr<__rpc_result> _impl;
    };

  } // rpc
} // atlas

#endif /* RFC_RESULT_H_ */

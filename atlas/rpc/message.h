/*
 * message.h
 *
 *  Created on: Oct 21, 2011
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
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

#ifndef ATLAS_RFC_MESSAGE_H_
#define ATLAS_RFC_MESSAGE_H_

#include <cstring>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace atlas {
  namespace rpc {

    using boost::uuids::uuid;

    enum return_type { rpc_sync, rpc_async_callback, rpc_async_no_callback };

    // TODO : check the alignment, when should be 4 and when 8? what's the difference?
#pragma pack(4)

    struct request_header {
      int32_t length;           // 1 the total length of this message
      int32_t fn_id;            // 2 function id
      int32_t return_type;      // 3 return type : see rpc::return_type
      int32_t client_id;        // 4 client id, indicate where the request comes from
      uuid    session_id;       // 5 the current session id
      int32_t resp_expect;      // 6 expected response count
    };

#pragma pack()

    std::ostream& operator<<(std::ostream& os, const request_header& h) {
      os << "l:" << h.length << ", c:" << h.client_id << ", s: " << h.session_id;

      return os;
    }

    class message {
    public:

      static const uuid& get_session_id(const char* data, size_t size = 0) {
        return reinterpret_cast<const request_header*>(data)->session_id;
      }

      static request_header make_header(int fn_id, const uuid& session_id) {
        return request_header {
          0,                                  // length
          fn_id,                              // method
          return_type::rpc_async_no_callback, // return type
          0,                                  // client id
          session_id,                         // session id
          1,                                  // resp_expect
        };
      }

    public:

      static const size_t request_header_size = sizeof(request_header);

      message(const std::string& data) : _header(*reinterpret_cast<const request_header*>(data.data())),
        _body(data.c_str() + request_header_size, data.size() - request_header_size) {}

      message(const char* data, size_t size) : _header(*reinterpret_cast<const request_header*>(data)),
        _body(data + request_header_size, size - request_header_size) {}

      void reset(const std::string& data) {
        reset(data.data(), data.size());
      }

      void reset(const char* data, size_t size) {
        std::memcpy(&_header, data, sizeof _header);
        _body.assign(data + request_header_size, size - request_header_size);
      }

      const request_header* header() const { return &_header; }

      const std::string& rpc_str() const { return _body; }

    private:

      request_header _header;
      std::string _body;
    };

  } // rpc
} // atlas

#endif /* RFC_MESSAGE_H_ */

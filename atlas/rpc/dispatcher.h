/*
 *  dispatcher.h
 *
 *  Created on: Oct 10, 2011
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

#ifndef RFC_DISPATCHER_H_
#define RFC_DISPATCHER_H_

#include <deque>

#include <boost/optional.hpp>
#include <atlas/serialization/uuid.h>
#include <atlas/rpc/rpc.h>

namespace atlas {
  namespace rpc {

    // dispatchers
    typedef std::function<boost::optional<rpc_result>(int, const std::string&, const rpc_context&)> dispatcher_type;

    class builtin_dispatcher {
    public:

      static boost::optional<rpc_result> dispatch(int fn_id, const std::string& message, const rpc_context& context) {
        std::istringstream iss(message);
        rpc_iarchive ia(iss);

        // std::cout << "dispatch " << fn_id << " for " << context.session_id() << " from " << context.source_ip();

        switch (fn_id) {
        case fn_ids::resume_thread: {
          rf_wrapper<decltype(builtin_rfc::resume_thread)> resume_thread(builtin_rfc::resume_thread, ia, context);
          return resume_thread();
        }
        break;
        case fn_ids::resume_task: {
          rf_wrapper<decltype(builtin_rfc::resume_task)> resume_task(builtin_rfc::resume_task, ia, context);
          return resume_task();
        }
        break;
        default:
          // not be responsible to this rpc
          return boost::none;
          // DLOG(INFO) << "no method " << method;
        break;
        } // switch

        return boost::none;
      }
    };

    class dispatcher_manager : public atlas::singleton<dispatcher_manager> {
    public:

      // TODO : make it private
      dispatcher_manager() { __init(); }

    public:

      // we invoke the dispatcher earlier if he comes later
      // TODO : employ priority queue
      void regist(dispatcher_type dispatcher) {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;

        _dispatchers.push_front(std::bind(dispatcher, _1, _2, _3));
      }

      // throw
      void execute(remote_caller& response_caller, const message& msg, const std::string& source_ip_port) {
        rpc_context context(msg.header()->client_id, msg.header()->return_type, msg.header()->session_id, source_ip_port);

        auto result = atlas::rpc::dispatcher_manager::dispatch(msg.header()->fn_id, msg.rpc_str(), context);
        if (result) respond(response_caller, context, result);
      }

      void respond(remote_caller& caller, const rpc_context& context, const rpc_result& result) {
        if (context.get_return_type() == rpc_async_callback) {
          caller.call(builtin_rfc::resume_task, fn_ids::resume_task, context.session_id(), result, nilctx);
        }
        else if (context.get_return_type() == rpc_sync) {
          caller.call(builtin_rfc::resume_thread, fn_ids::resume_thread, context.session_id(), result, nilctx);
        }
      }

      rpc_result dispatch(int fn_id, const std::string& message, const rpc_context& context) {
        std::istringstream iss(message);
        rpc_iarchive ia(iss);

        for (const dispatcher_type& dispatcher : _dispatchers) {
          auto result = dispatcher(fn_id, message, context);

          if (result) return *result; // got a proper processor
        }

        return nullptr; // no any proper processor
      }

    protected:

      void __init() {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;

        _dispatchers.push_front(std::bind(builtin_dispatcher::dispatch, _1, _2, _3));
      }

      std::deque<dispatcher_type> _dispatchers;
    };

  } // rpc
} // atlas

// TODO : use meta programming
#define ATLAS_REGISTER_RPC_DISPATCHER(module_name, dispatcher) \
namespace atlas { \
  namespace rpc { \
    namespace rpc_dispatcher_registers_for_module_##module_name { \
        class dispatcher_register { \
        public: \
          dispatcher_register() { \
            dispatcher_manager::ref().regist(dispatcher); \
          } \
        }; \
        dispatcher_register register_rpc_for_module_##module_name; \
    } \
  } \
}

#endif // RFC_DISPATCHER_H_

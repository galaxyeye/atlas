/*
 * sender.h
 *
 *  Created on: Apr 7, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_REMOTE_CALLER_H_
#define ATLAS_RPC_REMOTE_CALLER_H_

#include <sstream>
#include <functional>
#include <mutex>
#include <map>

#include <atlas/singleton.h>
#include <atlas/rpc/message.h>

namespace atlas {
  namespace rpc {

    typedef std::function<void(std::string, int)> rpc_callback;

    struct task {
      uuid id;
      rpc_callback cb;
    };

    class task_holder : singleton<task_holder> {
    public:

      void suspend(task&& task) {
        std::lock_guard<std::mutex> guard;

        _tasks.insert(std::make_pair(task.id, task));
      }

      void resume(const uuid& id, const std::string data, int err) {
        std::lock_guard<std::mutex> guard;

        auto it = _tasks.find(id);
        if (it != _tasks.end()) {
          if (it->second.cb) it->second.cb(data, err);
          _tasks.erase(it);
        }
      }

    private:

      task_holder() = default;
      friend class singleton<task_holder>;

    private:

      mutable std::mutex _mutex;
      std::map<uuid, task> _tasks;
    };

    class caller {
    public:

      template<typename Client>
      caller(Client c) : _client(c) {}

      template<typename Callable, typename Tuple>
      void call(Callable& c, rpc_callback cb, Tuple&& t) {
        message_builder builder(cb ? procedure_type::async_non_void : procedure_type::async_void);

        task_holder::instance().suspend(task{builder.session_id, cb});

        _client->send(builder(t));
      }

      template<typename Callable, typename Tuple>
      void call(Callable& c, Tuple&& t) {
        call(c, nullptr, t);
      }

      template<typename Callable, typename... Args>
      void call(Callable& c, rpc_callback cb, Args&& args...) {
        call(c, cb, std::forward_as_tuple(args...));
      }

      template<typename Callable, typename... Args>
      void call(Callable& c, Args&& args...) {
        call(c, nullptr, std::forward_as_tuple(args...));
      }

    private:

      client* _client;
    };

  } // rpc
} // atlas

#endif /* ATLAS_RPC_REMOTE_CALLER_H_ */

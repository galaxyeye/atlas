/*
 * caller.h
 *
 *  Created on: Apr 7, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_CALLER_H_
#define ATLAS_RPC_CALLER_H_

#include <sstream>
#include <functional>
#include <type_traits>
#include <mutex>
#include <future>
#include <map>
#include <memory>

#include <boost/blank.hpp>
#include <boost/optional.hpp>

#include <atlas/singleton.h>
#include <atlas/apply_tuple.h>
#include <atlas/rpc/message.h>

namespace atlas {
  namespace rpc {

    typedef std::function<void(std::string, int)> rpc_callback;

    struct task {
      uuid id;
      rpc_callback cb;
    };

    class async_task_holder : singleton<async_task_holder> {
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

      async_task_holder() = default;
      friend class singleton<async_task_holder>;

    private:

      mutable std::mutex _mutex;
      std::map<uuid, task> _tasks;
    };

    class sync_task_holder : singleton<sync_task_holder> {
    public:

      typedef std::promise<std::string> promise_type;
      typedef std::shared_ptr<promise_type> promise_ptr;

    public:

      std::string suspend(const uuid& id) {
        promise_ptr p(new promise_type);
        _promises.insert(std::make_pair(id, p));
        return p->get_future().get();
      }

      void resume(const uuid& id, const std::string data, int err) {
        promise_ptr p;

        auto it = _promises.find(id);
        if (it != _promises.end()) {
          p = *it;
          _promises.erase(it);
        }

        if (p) p->set_value(data);
      }

    private:

      sync_task_holder() = default;
      friend class singleton<sync_task_holder>;

    private:

      std::map<uuid, promise_ptr> _promises;
    };

    namespace {

      using std::enable_if;
      using std::is_void;
      using std::nullptr_t;

      template<typename F, typename Tuple>
      struct enable_if_void_call {
        typedef typename enable_if<is_void<typename declrettype<F, Tuple>::type>::value, nullptr_t>::type type;
      };

      template<typename F, typename Tuple>
      struct enable_if_non_void_call {
        typedef typename enable_if<!is_void<typename declrettype<F, Tuple>::type>::value, nullptr_t>::type type;
      };
    }

    class caller {
    public:

      template<typename Client>
      caller(Client client) : _client(std::addressof(client)) {}

      template<typename F, typename Tuple>
      void call(F& f, rpc_callback cb, Tuple&& t) {
        message_builder builder(cb ? procedure_type::async_callback : procedure_type::async_no_callback);
        _client->send(builder(t));

        if (cb) async_task_holder::ref().suspend(task{builder.session_id, cb});
      }

      template<typename F, typename Tuple>
      void call(F& f, Tuple&& t) { call(f, nullptr, t); }

      template<typename F, typename... Args>
      void call(F& f, rpc_callback cb, Args&& args...) { call(f, cb, std::forward_as_tuple(args...)); }

      template<typename F, typename... Args>
      void call(F& f, Args&& args...) { call(f, nullptr, std::forward_as_tuple(args...)); }

      template<typename F, typename Tuple>
      typename declrettype<F, Tuple>::type
      sync_call(F& f, Tuple&& t) {
        message_builder builder(procedure_type::sync);

        _client->send(builder(t));

        return sync_task_holder::ref().suspend(builder.session_id);
      }

      template<typename F, typename... Args>
      typename std::result_of<F(Args...)>::type
      sync_call(F& f, Args&& args...) { sync_call(f, std::forward_as_tuple(args...)); }

    private:

      client* _client;
    };

  } // rpc
} // atlas

#endif /* ATLAS_RPC_CALLER_H_ */

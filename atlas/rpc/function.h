/*
 * caller.h
 *
 *  Created on: Apr 7, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RFC_FUNCTION_H_
#define ATLAS_RFC_FUNCTION_H_

#include <sstream>
#include <functional>
#include <string>
#include <functional>
#include <type_traits>
#include <mutex>
#include <future>
#include <map>
#include <memory>

#include <boost/optional.hpp>
#include <boost/fusion/include/at_c.hpp>

#include <atlas/singleton.h>
#include <atlas/type_traits.h>
#include <atlas/apply_tuple.h>
#include <atlas/rfc/message.h>

namespace atlas {
  namespace rfc {

    template<typename CallVec>
    struct executor {

      executor(CallVec& calls) : calls(calls) {}

      // TODO : return type
      template<size_t id, typename... Args>
      void operator()(Args&&... args) {
        boost::fusion::at_c<id>(calls)(std::forward<Args>(args)...);
      }

      CallVec& calls;
    };

    typedef std::promise<std::string> remote_promise;
    typedef std::shared_ptr<remote_promise> remote_promise_ptr;
    typedef std::function<void(const std::string&, int)> rpc_callback;

    struct task {
      uuid id;
      union {
        rpc_callback call;
        remote_promise_ptr promise;
      };
    };

    struct sync_tag {};

    class task_holder : singleton<task_holder> {
    public:

      void suspend(task&& task) {
        std::lock_guard<std::mutex> guard;

        _tasks.insert(std::make_pair(task.id, task));
      }

      const std::string& suspend(const uuid& id, sync_tag) {
        remote_promise_ptr promise;

        {
          std::lock_guard<std::mutex> guard;
          _tasks.insert(std::make_pair(id, task{id, promise}));
        }

        return promise->get_future().get();
      }

      void resume(const uuid& id, const std::string data, int err) {
        std::lock_guard<std::mutex> guard;

        auto it = _tasks.find(id);
        if (it != _tasks.end()) {
          if (it->second.call) {
            it->second.call(data, err);
            if (it->second.promise) it->second.promise->set_value(data);
            _tasks.erase(it);
          }
        }
      }

    private:

      task_holder() = default;
      friend class singleton<task_holder>;

    private:

      mutable std::mutex _mutex;
      std::map<uuid, task> _tasks; // TODO : concurrent data structure for better performance
    };

    template<typename Client, typename FnIdx>
    class caller {
    public:

      caller(Client& client) : _client(std::addressof(client)) {}

      template<typename F, typename... Args>
      void call(F& f, rpc_callback cb, Args&& args...) { call(f, cb, std::forward_as_tuple(args...)); }

      template<typename F, typename... Args>
      void call(F& f, Args&& args...) { call(f, nullptr, std::forward_as_tuple(args...)); }

      template<typename F, typename... Args>
      typename std::result_of<F(Args...)>::type
      sync_call(F& f, Args&& args...) { sync_call(f, std::forward_as_tuple(args...)); }

    protected:

      template<typename F, typename Tuple>
      void call(F& f, rpc_callback cb, Tuple&& t) {
        message_builder builder(cb ? procedure_type::async_callback : procedure_type::async_no_callback);

        using boost::fusion::at_key;
        auto id = at_key<std::remove_reference<F>>(_index);
        _client->send(builder(id, t));

        if (cb) task_holder::ref().suspend(task{builder.session_id, cb});
      }

      template<typename F, typename Tuple>
      void call(F& f, Tuple&& t) { call(f, nullptr, t); }

      template<typename F, typename Tuple>
      typename declrtype<F, Tuple>::type
      sync_call(F& f, Tuple&& t) {
        typedef typename declrtype<F, Tuple>::type result_type;

        using boost::fusion::at_key;
        auto id = at_key<std::remove_reference<F>>(_index);

        message_builder builder(procedure_type::sync);
        _client->send(builder(id, t));

        auto result = task_holder::ref().suspend(builder.session_id, sync_tag());
        return boost::lexical_cast<result_type>(result);
      }

    private:

      Client* _client;
      FnIdx _index;
    };

  } // rpc
} // atlas

#endif /* ATLAS_RFC_FUNCTION_H_ */

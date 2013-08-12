/*
 * task.h
 *
 *  Created on: Sep 3, 2011
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

#ifndef ATLAS_RPC_TASK_H_
#define ATLAS_RPC_TASK_H_

#include <map>
#include <string>
#include <mutex>
#include <functional>
#include <future>

#include <boost/uuid/uuid.hpp>

#include <atlas/singleton.h>
#include <atlas/rpc/result.h>

namespace atlas {
  namespace rpc {

    using boost::uuids::uuid;

    class async_task;
    typedef std::function<void(const std::string&, int, async_task& task)> rpc_callback_type;

    struct __async_task {

      __async_task(rpc_callback_type cb = nullptr, int response_received = 1, int response_expected = 0)
        : cb(cb), response_received(response_received), response_expected(response_expected), record_count(0) {}

      __async_task(const __async_task& d)
        : cb(d.cb), response_received(d.response_received), response_expected(d.response_expected),
          record_count(0), data_list(d.data_list) {}

      rpc_callback_type cb;
      int response_received;
      int response_expected;
      size_t record_count;
      std::vector<std::string> data_list;
    };

    class async_task {
    public:

      async_task(std::nullptr_t) {}

      async_task(rpc_callback_type cb = nullptr, int response_received = 1)
        : _pimpl(new __async_task(cb, response_received)) {}

      async_task(const async_task& task) :
        _pimpl(new __async_task(*task._pimpl))
      {}

      async_task& operator=(const async_task& task) {
        if (std::addressof(task) != this) {
          _pimpl.reset(new __async_task());

          _pimpl->cb = task._pimpl->cb;
          _pimpl->response_received = task._pimpl->response_received;
          _pimpl->response_received = task._pimpl->response_received;
          _pimpl->record_count = task._pimpl->record_count;
          _pimpl->data_list = task._pimpl->data_list;
        }

        return *this;
      }

      async_task& operator=(async_task&& task) {
        if (std::addressof(task) != this) {
          std::swap(_pimpl, task._pimpl);
        }

        return *this;
      }

      operator bool() const { return _pimpl.operator bool(); }

      void increase_response() { ++_pimpl->response_received; }

      bool ready() const { return _pimpl->response_received == _pimpl->response_received; }

      void run(const std::string& result, int err) {
        if (_pimpl->cb) _pimpl->cb(result, err, *this);
      }

      void put_data(const std::string& data) { _pimpl->data_list.push_back(data); }

      void put_data(std::string&& data) { _pimpl->data_list.push_back(data); }

      size_t response_count() const { return _pimpl->response_received; }

      size_t expected_response_count() const { return _pimpl->response_expected; }

      size_t record_count() const { return _pimpl->record_count; }

      std::string merge_data(char sep = '\0') {
        std::string result;
        for (const auto& s : _pimpl->data_list) {
          result += s;
          if (sep != '\0') result += sep;
        }
        return result; // NRVO
      }

      const std::vector<std::string>& data_list() const { return _pimpl->data_list; }

    private:

      std::shared_ptr<__async_task> _pimpl;
    };

    inline std::ostream& operator<<(std::ostream& os, const async_task& task) {
      os << task.response_count() << ", " << task.expected_response_count();
      return os;
    }

    class sync_task_manager : public atlas::singleton<sync_task_manager> {
    public:

      typedef std::shared_ptr<std::promise<rpc_result>> promise_ptr;

    public:

      rpc_result suspend(const uuid& id) {
        promise_ptr promise(new std::promise<rpc_result>());

        {
          std::lock_guard<std::mutex> guard(_mutex);
          _promises.insert(std::make_pair(id, promise));
        }

        std::future<rpc_result> future = promise->get_future();
        future.wait();

        return std::move(future.get());
      }

      void resume(const uuid& id, const std::string& result, int err_code = 0) {
        promise_ptr promise;

        {
          std::lock_guard<std::mutex> guard(_mutex);
          promise = _promises[id];
          _promises.erase(id);
        }

        rpc_result r(result, err_code);
        promise->set_value(r);
      }

      void clear() {
        std::lock_guard<std::mutex> guard(_mutex);
        _promises.clear();
      }

    private:

      std::mutex _mutex;
      std::map<uuid , promise_ptr> _promises;
    };

    class async_task_manager : public atlas::singleton<async_task_manager> {
    public:

      void suspend(const uuid& id, rpc_callback_type cb, int response_received = 1) {
        std::lock_guard<std::mutex> guard(_mutex);

        async_task task(cb, response_received);
        _sessions.insert(std::make_pair(id, task));
      }

      void resume(const uuid& id, const std::string& result, int err_code = 0) {
        std::lock_guard<std::mutex> guard(_mutex);

        auto it = _sessions.find(id);
        if (it != _sessions.end()) {
          // increase response counter
          it->second.increase_response();

          // call callback
          it->second.run(result, err_code);

          // clean if need
          if (it->second.ready()) {
            _sessions.erase(it);
          }
        }
      }

    private:

      std::mutex _mutex;
      std::map<uuid, async_task> _sessions;
    };

  } // rpc
} // atlas

#endif /* ATLAS_TASK_H_ */

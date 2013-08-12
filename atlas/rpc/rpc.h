/*
 * rpc.h
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

#ifndef ATLAS_RPC_RPC_H_
#define ATLAS_RPC_RPC_H_

#include <string>
#include <functional>
#include <tuple>

#include <boost/lexical_cast.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

#ifdef DEBUG_RPC

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#else

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#endif

#include <atlas/serialization/tuple.h>
#include <atlas/apply_tuple.h>

#include <atlas/rpc/message.h>
#include <atlas/rpc/task.h>

namespace atlas {
  namespace rpc {

    using std::string;
    using boost::uuids::uuid;
    using boost::uuids::nil_uuid;
    using boost::uuids::random_generator;

#ifdef ATLAS_DEBUG_RPC

    typedef boost::archive::text_iarchive rpc_iarchive;
    typedef boost::archive::text_oarchive rpc_oarchive;

#else

    typedef boost::archive::binary_iarchive rpc_iarchive;
    typedef boost::archive::binary_oarchive rpc_oarchive;

#endif

#define REGISTER_REMOTE_FUNC(func_name, func_id) namespace fn_ids { \
    static const int func_name = func_id; \
};

    namespace {
      struct useless {};
    }

    template<typename... T>
    class rf_wrapper;

    // this class is similar to atlas::serialization::function, except that the rf_wrapper takes an extra
    // parameter when it de-serializes, which used to pass a local context to the function call before it executes
    template<typename Res, typename... Args>
    class rf_wrapper<Res(Args...)> {
    public:

      rf_wrapper() = default;

      rf_wrapper(const rf_wrapper& other) : _args(other._args), _f(other._f) {}

      rf_wrapper(rf_wrapper&& other) : _args(std::move(other._args)), _f(std::move(other._f)) {}

      // TODO : add OArchive concept check
      template<typename Functor, typename OArchiver>
      rf_wrapper(Functor f, Args... args, OArchiver& ar,
          typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless()) :
          _args(args...), _f(f)
      {
        ar << _args;
      }

      rf_wrapper& operator=(const rf_wrapper& other) {
        rf_wrapper(other).swap(*this);
        return *this;
      }

      rf_wrapper& operator=(rf_wrapper&& other) {
        rf_wrapper(std::move(other)).swap(*this);
        return *this;
      }

      /*
       * The last parameter can be replaced by a local variable
       * TODO : add IArchive concept check
       * */
      template<typename Functor, typename IArchiver, typename NativeArg>
      rf_wrapper(Functor f, IArchiver& ia, const NativeArg& native_arg,
          typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless()) :
          _f(f)
      {
        ia >> _args;
        std::get<sizeof...(Args) - 1>(_args) = native_arg;
      }

      void swap(rf_wrapper& other) {
        std::swap(_args, other._args);
        std::swap(_f, other._f);
      }

      explicit operator bool() const { return _f.operator bool();}

      Res operator()() const { return apply_tuple(_f, _args); }

    private:

      std::tuple<typename std::decay<Args>::type...> _args;
      std::function<Res (Args...)> _f;
    };

    struct __rpc_context {

      __rpc_context() : client_id(0), rt(return_type::rpc_async_no_callback) {}

      __rpc_context(int client_id, int rt, const uuid& session_id, const std::string& source_ip_port) :
        client_id(client_id), rt(rt), session_id(session_id), source_ip_port(source_ip_port)
      {}

      __rpc_context(const __rpc_context& other)
        : client_id(other.client_id), rt(other.rt), session_id(other.session_id), source_ip_port(other.source_ip_port)
      {}

      int client_id;
      int rt;
      uuid session_id;
      std::string source_ip_port;
    };

    class rpc_context {
    public:

      rpc_context(std::nullptr_t) {}

      rpc_context() : _impl(new __rpc_context) {}

      rpc_context(int client_id, int return_type, const uuid& session_id, const std::string& source_ip_port) :
          _impl(new __rpc_context(client_id, return_type, session_id, source_ip_port)) {
      }

      rpc_context(const rpc_context& other) : _impl(other._impl ? new __rpc_context(*other._impl)  : nullptr) {
      }

      rpc_context(rpc_context&& other) : _impl(std::move(other._impl)) {}

      rpc_context operator=(const rpc_context& other) {
        if (std::addressof(other) == this) return *this;

        if (other._impl) _impl.reset(new __rpc_context(*other._impl));

        return *this;
      }

      rpc_context operator=(rpc_context&& other) {
        if (std::addressof(other) == this) return *this;

        if (other._impl) _impl = other._impl;

        return *this;
      }

      void reset(int client_id, int return_type, const uuid& session_id, const std::string& source_ip_port) {
        _impl.reset(new __rpc_context(client_id, return_type, session_id, source_ip_port));
      }

      int client_id() const { return _impl->client_id; }

      int get_return_type() const { return _impl->rt; }

      const uuid& session_id() const { return _impl->session_id; }

      std::string source_ip() const { return _impl->source_ip_port.substr(0, _impl->source_ip_port.find(":")); }

      const std::string& source_ip_port() const { return _impl->source_ip_port; }

    private:

      std::shared_ptr<__rpc_context> _impl;
    };

    // constant null value
    const rpc_context nilctx(nullptr);

    class builtin_rfc {
    public:

      static rpc_result resume_thread(const uuid& sid, const rpc_result& result, const rpc_context& c) noexcept {
        sync_task_manager::ref().resume(sid, result.data(), result.err());

        return nullptr;
      }

      static rpc_result resume_task(const uuid& sid, const rpc_result& result, const rpc_context& c) noexcept {
        async_task_manager::ref().resume(sid, result.data(), result.err());

        return nullptr;
      }
    };

    // builtin rpc
    REGISTER_REMOTE_FUNC(resume_thread, -1);
    REGISTER_REMOTE_FUNC(resume_task, -2);

  } // rpc
} // atlas

namespace boost {
  namespace serialization {

    template<typename Archive>
    void serialize(Archive& ar, atlas::rpc::rpc_context& c, const unsigned int version) {
      // NOTE : Nothing to do, this is just a placeholder in the serialization system
    }

    template<class Archive>
    void serialize(Archive & ar, atlas::rpc::rpc_result& r, const unsigned int file_version) {
      split_free(ar, r, file_version);
    }

    template<class Archive>
    void load(Archive& ar, atlas::rpc::rpc_result& r, const unsigned int) {
      std::string data;
      int err = 0;

      ar >> data;
      ar >> err;

      r.reset(data, err);
    }

    template<class Archive>
    void save(Archive& ar, const atlas::rpc::rpc_result& r, const unsigned int) {
      std::string data = r.data();
      int err = r.err();

      ar << data;
      ar << err;
    }

  } // serialization
} // boost

namespace atlas {
  namespace rpc {

    class message_builder {
    public:

      message_builder(int client) : _client_id(client), _return_type(rpc_async_no_callback) {}

    public:

      const uuid& session_id() const { return _session_id; }

      void set_return_type(return_type rt) { _return_type = rt; }

      template<typename Functor, typename ... Args>
      std::string build(Functor f, int fn_id, Args&&... args) {
        typedef typename std::result_of<Functor(Args&&...)>::type result_type;

        _session_id = random_generator()();
        request_header header = message::make_header(fn_id, _session_id);
        header.client_id = _client_id;
        header.return_type = _return_type;

        std::ostringstream oss;
        std::string header_str(reinterpret_cast<char*>(&header), sizeof(header));

        rpc_oarchive oa(oss);
        rf_wrapper<result_type(Args...)> rpc(f, std::forward<Args>(args)..., oa);

        std::string body = oss.str();
        std::string message(header_str);
        message += body;

        auto h = reinterpret_cast<request_header*>(const_cast<char*>(message.data()));
        h->length = message.size();

        return std::move(message);
      }

    private:

      int _client_id;
      return_type _return_type;
      uuid _session_id;
    };

    // the user must create callers inherit from this, and implement the pure virtual function : send
    class remote_caller {
    public:

      remote_caller(int client = 0, int response_expected = 1) :
        _message_builder(client), _response_expected(response_expected)
      {}

      virtual ~remote_caller() {}

    public:

      /*
       * 1) Serialize a remote function call with it's arguments,
       * 2) send the message to the target using the derived class's implementation
       * */
      template<typename Functor, typename ... Args>
      void call(Functor f, int fn_id, Args ... args) {
        _message_builder.set_return_type(rpc_async_no_callback);

        send(_message_builder.build(f, fn_id, std::forward<Args>(args)...));
      }

      /*
       * 1) serialize a remote function call with arguments,
       * 2) save the context to wait for the result
       * 3) register the callback for the returning
       * 4) send the message to the target using the derived class's implementation
       * 5) the server should issue a resume_task function call and then the client
       *    calls the callback
       * */
      template<typename Functor, typename ... Args>
      void call(Functor f, int fn_id, rpc_callback_type cb, Args ... args) {
        _message_builder.set_return_type(rpc_async_callback);

        std::string message = _message_builder.build(f, fn_id, std::forward<Args>(args)...);
        async_task_manager::ref().suspend(_message_builder.session_id(), cb, _response_expected);

        send(message);
      }

      /*
       * The remote_caller thread will be blocked to wait for the result
       * */
      template<typename Functor, typename ... Args>
      rpc_result sync_call(Functor f, int fn_id, Args ... args) {
        typedef typename std::result_of<Functor(Args...)>::type result_type;
        _message_builder.set_return_type(rpc_sync);

        std::string message = _message_builder.build(f, fn_id, std::forward<Args>(args)...);
        rpc_result rr = sync_task_manager::ref().suspend(_message_builder.session_id());

        send(message);

        return std::move(rr);
      }

    protected:

      void send(const std::string& message) {
        send(message.data(), message.size());
      }

      // override this function to optimize at network layer
      virtual void send(std::string&& message) {
        send(message.data(), message.size());
      }

      virtual void send(const char* message, size_t size) = 0;

    private:

      message_builder _message_builder;
      int _response_expected;
    };


  } // rpc
} // atlas

#endif /* ATLAS_RFC_DETAIL_RFC_H_ */

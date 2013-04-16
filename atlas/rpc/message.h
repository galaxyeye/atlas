/*
 * message.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_MESSAGE_H_
#define ATLAS_RPC_MESSAGE_H_

#include <type_traits>
#include <tuple>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace atlas {
  namespace rpc {

    using boost::uuids::uuid;
    using boost::uuids::random_generator;
    using boost::archive::binary_oarchive;

    enum class procedure_type : int8_t { sync, async_callback, async_no_callback };

    enum class default_procedure_id : int8_t { test, resume_task, resume_thread };

    struct rpc_header {
      procedure_type type;
      uuid id;
    };

    struct rpc_result {
      std::string data;
      int error;
    };

    struct message_builder {

      message_builder(procedure_type type) : session_id(random_generator()()), type(type) {}

      // enable if Tuple is a tuple
      template<typename Tuple>
      std::string operator()(Tuple&& t) {
        std::ostringstream oss;
        binary_oarchive oa(oss);

        oa << type << session_id << t;

        return oss.str();
      }

      template<typename... Args>
      std::string operator()(Args&&... args) {
        return operator()(std::forward_as_tuple(args...));
      }

      uuid session_id;
      procedure_type type;
    };

  } // rpc
} // atlas

#endif /* MESSAGE_H_ */

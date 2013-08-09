/*
 * message.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RFC_MESSAGE_H_
#define ATLAS_RFC_MESSAGE_H_

#include <type_traits>
#include <tuple>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

namespace atlas {
  namespace rfc {

    using boost::uuids::uuid;
    using boost::uuids::random_generator;
    using boost::archive::binary_oarchive;
    using boost::archive::binary_iarchive;

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

      // TODO : enable if Tuple is a std::tuple
      template<typename Tuple>
      std::string operator()(size_t id, Tuple&& t) {
        std::ostringstream oss;
        binary_oarchive oa(oss);

        oa << type << session_id << id << t;

        return oss.str();
      }

      template<typename... Args>
      std::string operator()(size_t id, Args&&... args) {
        return operator()(std::forward_as_tuple(args...));
      }

      uuid session_id;
      procedure_type type;
    };

    struct message_extractor {

      template<typename Tuple>
      void operator()(const std::string& message, Tuple& t) {
        std::istringstream iss(message);
        binary_iarchive ia(iss);

        ia >> type >> session_id >> id >> t;
      }

//      template<typename... Args>
//      void operator()(size_t id, Args&&... args) {
//        return operator()(std::forward_as_tuple(args...));
//      }

      size_t id;
      uuid session_id;
      procedure_type type;
    };

  } // rpc
} // atlas

#endif /* MESSAGE_H_ */

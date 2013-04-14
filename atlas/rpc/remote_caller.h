/*
 * sender.h
 *
 *  Created on: Apr 7, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_REMOTE_CALLER_H_
#define ATLAS_RPC_REMOTE_CALLER_H_

#include <functional>
#include <type_traits>
#include <sstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace atlas {
  namespace rpc {

    using boost::uuids::uuid;
    using boost::uuids::random_generator;
    using boost::archive::binary_oarchive;

    class remote_caller {
    public:

      template<typename MessageSender>
      remote_caller(const MessageSender& sender) : _sender(sender) {}

      template<typename Callable, typename Tuple>
      void call(const Callable& c, Tuple&& t) {
        std::ostringstream oss;
        binary_oarchive oa(oss);

        random_generator gen;
        oa << gen() << t;

        _sender.send(oss.str());
      }

    private:

      const message_sender& _sender;
    };

  } // rpc
} // atlas

#endif /* ATLAS_RPC_REMOTE_CALLER_H_ */

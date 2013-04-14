/*
 * message_sender.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_MESSAGE_SENDER_H_
#define ATLAS_RPC_MESSAGE_SENDER_H_

#include <string>
#include <boost/asio/buffer.hpp>

namespace atlas {
  namespace rpc {

    class message_sender {
    public:

      virtual ~message_sender() {}

      virtual void send(const char* message, size_t size) = 0;

      virtual void send(const std::string& message) = 0;

    };

  }
}


#endif /* MESSAGE_SENDER_H_ */

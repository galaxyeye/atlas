/*
 * message.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_MESSAGE_H_
#define ATLAS_RPC_MESSAGE_H_

#include <boost/uuid/uuid.hpp>

namespace atlas {
  namespace rpc {

    using boost::uuids::uuid;

    enum class procedure_type : int8_t { sync_non_void, sync_void, async_non_void, async_void };

    enum class procedure_id : int8_t { test, resume_task, resume_thread };

    struct rpc_header {
      int16_t length;
      uuid id;
      int8_t type;
    };

    struct rpc_result {
      std::string result;
      int error;
    };
  }
}

#endif /* MESSAGE_H_ */

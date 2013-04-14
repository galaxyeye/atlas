/*
 * dispatcher.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_DISPATCHER_H_
#define ATLAS_RPC_DISPATCHER_H_

#include <message.h>

namespace atlas {
  namespace rpc {

    class dispatcher {
    public:

      static rpc_result dispatch(procedure_id id, std::istringstream& is) {
        rpc_result result;

        switch(procedure_id) {
        case procedure_id::test:
          break;
        case procedure_id::resume_task:
          break;
        case procedure_id::resume_thread:
          break;
        default:
          break;
        }

        return result;
      }
    };

  } // rpc
} // atlas

#endif /* ATLAS_RPC_DISPATCHER_H_ */

/*
 * dispatcher.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_DISPATCHER_H_
#define ATLAS_RPC_DISPATCHER_H_

#include <atlas/rpc/callable.h>
#include <atlas/rpc/message.h>

namespace atlas {
  namespace rpc {

    template<>
    struct rpc_dispacher{};

    class dispatcher {
    public:

      static rpc_result dispatch(int id, std::istringstream& is) {
        rpc_result result;

        return result;
      }

    private:

      callable_map _callable_map;
    };

  } // rpc
} // atlas

#endif /* ATLAS_RPC_DISPATCHER_H_ */

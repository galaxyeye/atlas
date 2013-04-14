/*
 * native_caller.h
 *
 *  Created on: Apr 7, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RPC_NATIVE_CALLER_H_
#define ATLAS_RPC_NATIVE_CALLER_H_

#include <string>

#include <message.h>

namespace atlas {
  namespace rpc {

    struct use_service {};

    class native_caller {
    public:

      template<typename Callable, typename Tuple>
      rpc_result call(const Callable& c, Tuple&& t) {
        apply_tuple(c, t);
      }

      template<typename Callable, typename... Args>
      rpc_result call(const Callable& c, Args&& args...) {
        c(args...);
      }

    private:

    };

  } // rpc
} // atlas

#endif /* NATIVE_CALLER_H_ */

/*
 * dispatcher.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RFC_DISPATCHER_H_
#define ATLAS_RFC_DISPATCHER_H_

#include <boost/preprocessor/repetition.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <atlas/rfc/message.h>

namespace atlas {
  namespace rfc {

    using boost::optional;

    template<size_t Id>
    struct invoke_ith {

      template<typename FnVec, typename FnIdx, typename ArgsList>
      void operator()(std::istringstream& is, FnVec& fns, FnIdx& fnidx) {
        using namespace boost::fusion;
        result_of::at_c<ArgsList, 0>::type args;
        is >> args;
        apply_tuple(at_c<Id>(fns), args);
      }
    };

    template<typename FnVec, typename FnIdx, typename ArgsList>
    class dispatcher {
    public:

      static optional<rpc_result> dispatch(size_t id, std::istringstream& iss) {
        optional<rpc_result> result;

//        static const auto proc_invokers = boost::fusion::make_vector(
//            try_invoke<0>(),
//            try_invoke<1>(),
//            try_invoke<2>(),
//            try_invoke<3>(),
//            try_invoke<4>(),
//            try_invoke<5>());
//        boost::fusion::for_each(proc_invokers, invoke);

        if (id == 0) invoke_ith<0>()(iss, _fns, _fnidx);
        if (id == 1) invoke_ith<1>()(iss, _fns, _fnidx);
        if (id == 2) invoke_ith<2>()(iss, _fns, _fnidx);
        if (id == 3) invoke_ith<3>()(iss, _fns, _fnidx);
        if (id == 4) invoke_ith<4>()(iss, _fns, _fnidx);
        if (id == 5) invoke_ith<5>()(iss, _fns, _fnidx);

        // ...
        // bad, try better
        // BOOST_PP_REPEAT？

        return result;
      }

      template<size_t Id, typename Tuple>
      void call(Tuple& tuple) {
        apply_tuple(boost::fusion::at_c<Id>(_fns), std::forward<Tuple>(tuple));
      }

      template<size_t Id, typename Tuple>
      void call(Tuple&& tuple) {
        apply_tuple(boost::fusion::at_c<Id>(_fns), std::forward<Tuple>(tuple));
      }

//      template<size_t id, typename... Args>
//      void call(Args&&... args) {
//        boost::fusion::at_c<id>(_fns)(std::forward(args)...);
//      }

      FnVec _fns;
      FnIdx _fnidx;
    };

  } // rpc
} // atlas

#endif /* ATLAS_RFC_DISPATCHER_H_ */

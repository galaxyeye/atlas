/*
 * callable.h
 *
 *  Created on: Apr 14, 2013
 *      Author: vincent
 */

#ifndef ATLAS_RFC_CALLABLE_H_
#define ATLAS_RFC_CALLABLE_H_

#include <string>
#include <functional>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/push_back.hpp>

#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/make_vector.hpp>
//#include <boost/fusion/sequence.hpp>
//#include <boost/fusion/container/vector.hpp>

namespace atlas {
  namespace rfc {
    namespace {

      // using boost::mpl::vector;
      using boost::mpl::pair;
      using boost::mpl::push_back;
      using std::nullptr_t;

      // typedef vector<std::function<void(int, int)>, void(int, int)> __callable_vector;

      template<typename Container, typename F, typename... Args>
      struct __add_callable {
        typedef typename push_back<Container, pair<F, Args...>>::type type;
      };

    }

    template<typename Container, typename F, typename... Args>
    using add_callable = __add_callable<Container, F, Args...>;

//    template<typename F, typename... Args>
//    typename boost::fusion::result_of::make_vector<F(Args...)>::type
//    make_call_vector(const F& f) {
//      return boost::fusion::result_of::make_vector<F(Args...)>(f);
//    }
//
//    template<typename F, typename... Args, typename F2, typename... Args2>
//    typename boost::fusion::result_of::make_vector<F(Args...), F2(Args2...)>::type
//    make_call_vector(const F& f) {
//      return boost::fusion::result_of::make_vector<F(Args...), F2(Args2...)>(f);
//    }
//
//    template<typename F, typename... Args, typename F2, typename... Args2, typename F3, typename... Args3>
//    typename boost::fusion::result_of::make_vector<F(Args...), F2(Args2...), F3(Args3...)>::type
//    make_call_vector(const F& f) {
//      return boost::fusion::result_of::make_vector<F(Args...), F2(Args2...), F3(Args3...)>(f);
//    }

    template<typename F>
    struct make_call_vector;

    template<typename F, typename... Args>
    struct make_call_vector<F(Args)> {

    };

    template<typename CallVec>
    struct caller {

      caller(CallVec& calls) : calls(calls) {}

      template<size_t id, typename F>
      void apply(F&& f) {
        boost::fusion::at_c<id>(calls) = f;
      }

      template<size_t id, typename... Args>
      void call(Args&&... args) {
        boost::fusion::at_c<id>(calls)(std::forward<Args>(args)...);
      }

      CallVec& calls;
    };


  } // rfc
} // atlas

#endif /* CALLABLE_H_ */

/*
 * callable.h
 *
 *  Created on: Apr 14, 2013
 *      Author: vincent
 */

#ifndef CALLABLE_H_
#define CALLABLE_H_

#include <boost/mpl/at.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/support/pair.hpp>

namespace atlas {

  namespace {

    using boost::fusion::map;
    using boost::fusion::pair;
    using std::nullptr_t;

    typedef map<pair<int, std::less<int>>,  pair<int, std::greater<int>>> __callable_map;
  }

  template<typename... Callable>
  struct register_callback {
    typedef Callable type;
  };

  typedef __callable_map callable_map;

  callable_map callable_map;

  template<int id, typename... Args>
  struct callable_dispatcher {
    typedef boost::mpl::at<callable_map, id>::type type;

    void operator()(Args... args) {
      callable_map<id>(args);
    }
  };

} // atlas

#endif /* CALLABLE_H_ */

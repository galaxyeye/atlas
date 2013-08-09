//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_ALGORITH_HPP
#define BOOST_TRANSACT_DETAIL_ALGORITH_HPP

#include <boost/mpl/bool.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/transact/detail/config.hpp>

namespace boost {
  namespace transact {
    namespace detail {

//same as mpl::for_each, but without constructing the element

      template<class It, class End, class F>
      void for_each(F &f, mpl::true_) {
      }

      template<class It, class End, class F>
      void for_each(F &f, mpl::false_) {
        typedef typename mpl::deref<It>::type Element;
        f.BOOST_NESTED_OPERATOR_TEMPLATE operator()<Element>();

        typedef typename mpl::next<It>::type Next;
        for_each<Next, End>(f, typename boost::is_same<Next, End>::type());
      }

      template<class Sequence, class F>
      void for_each(F f) {
        typedef typename mpl::begin<Sequence>::type Begin;
        typedef typename mpl::end<Sequence>::type End;
        for_each<Begin, End>(f, typename boost::is_same<Begin, End>::type());
      }

//same as for_each, but stop iterating when the function returns false

      template<class It, class End, class F>
      bool for_each_if(F &f, mpl::true_) {
        return true;
      }

      template<class It, class End, class F>
      bool for_each_if(F &f, mpl::false_) {
        typedef typename mpl::deref<It>::type Element;
        if (f.BOOST_NESTED_OPERATOR_TEMPLATE operator()<Element>()) {
          typedef typename mpl::next<It>::type Next;
          return for_each_if<Next,End>(f,typename boost::is_same<Next,End>::type());
        }
        else return false;
      }

      template<class Sequence, class F>
      bool for_each_if(F f) {
        typedef typename mpl::begin<Sequence>::type Begin;
        typedef typename mpl::end<Sequence>::type End;
        return for_each_if<Begin, End>(f, typename boost::is_same<Begin, End>::type());
      }

      template<class It, class End, class State, class F>
      State fold(F &, State const &state, mpl::true_) {
        return state;
      }

      template<class It, class End, class State, class F>
      State fold(F &f, State const &state, mpl::false_) {
        typedef typename mpl::deref<It>::type Element;
        typedef typename mpl::next<It>::type Next;
        return fold<Next, End>(f, f.BOOST_NESTED_OPERATOR_TEMPLATE operator()<Element>(state),typename boost::is_same<Next,End>::type());
      }

      template<class Sequence, class State, class F>
      State fold(F f, State const &initial) {
        typedef typename mpl::begin<Sequence>::type Begin;
        typedef typename mpl::end<Sequence>::type End;
        return fold<Begin, End>(f, initial, typename boost::is_same<Begin, End>::type());
      }

    }
  }
}

#endif

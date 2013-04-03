/*
 * apply_tuple.h
 *
 *  Created on: Apr 1, 2013
 *      Author: vincent
 */

/*
 * Copyright 2013 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Defines a function atlas::applyTuple, which takes a function and a
 * std::tuple of arguments and calls the function with those
 * arguments.
 *
 * Example:
 *
 *    int x = atlas::applyTuple(std::plus<int>(), std::make_tuple(12, 12));
 *    ASSERT(x == 24);
 */

#ifndef ATLAS_APPLY_TUPLE_H_
#define ATLAS_APPLY_TUPLE_H_

namespace atlas {

  namespace {

    // This is to allow using this with pointers to member functions,
    // where the first argument in the tuple will be the this pointer.
    template<typename F> F& make_callable(F& f) {
      return f;
    }

    template<typename R, typename C, typename ...A>
    auto make_callable(R (C::*d)(A...)) -> decltype(std::mem_fn(d)) {
      return std::mem_fn(d);
    }

    template<typename Tuple>
    struct deref_size: std::tuple_size<typename std::remove_reference<Tuple>::type> {
    };

    // call_tuple recursively unpacks tuple arguments so we can forward
    // them into the function.
    template<typename Ret>
    struct call_tuple {

      template<typename F, typename Tuple, typename ...Unpacked>
      static typename std::enable_if<(sizeof...(Unpacked) < deref_size<Tuple>::value), Ret>::type
      call(const F& f, Tuple&& t, Unpacked&&... unp) {
        typedef typename std::tuple_element<sizeof...(Unpacked),
            typename std::remove_reference<Tuple>::type>::type ElementType;

        return call_tuple<Ret>::call(f, std::forward<Tuple>(t),
            std::forward<Unpacked>(unp)...,
            std::forward<ElementType>(std::get<sizeof...(Unpacked)>(t))
        );
      }

      template<typename F, typename Tuple, typename ...Unpacked>
      static typename std::enable_if<(sizeof...(Unpacked) == deref_size<Tuple>::value), Ret>::type
      call(const F& f, Tuple&& t, Unpacked&&... unp) {
        return make_callable(f)(std::forward<Unpacked>(unp)...);
      }
    };

    // The point of this meta function is to extract the contents of the
    // tuple as a parameter pack so we can pass it into std::result_of<>.
    template<typename F, typename Args> struct return_value { };

    template<typename F, typename ...Args>
    struct return_value<F, std::tuple<Args...>> {
      typedef typename std::result_of<F(Args...)>::type type;
    };
  } // anonymous

  template<typename Callable, typename Tuple>
  typename return_value<typename std::decay<Callable>::type, typename std::remove_reference<Tuple>::type>::type
  apply_tuple(const Callable& c, Tuple&& t) {
    typedef typename return_value<typename std::decay<Callable>::type,
        typename std::remove_reference<Tuple>::type>::type RetT;
    return call_tuple<RetT>::call(c, std::forward<Tuple>(t));
  }

} // atlas

#endif /* APPLY_TUPLE_H_ */

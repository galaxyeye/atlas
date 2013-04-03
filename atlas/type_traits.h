/*
 * type_traits.h
 *
 *  Created on: Apr 1, 2013
 *      Author: vincent
 */

#ifndef ATLAS_TYPE_TRAITS_H_
#define ATLAS_TYPE_TRAITS_H_

#include <type_traits>

namespace atlas {

  namespace {

    template<typename ... Elements>
    struct __is_single_parameter_pack_helper {
      typedef typename std::conditional<1 == sizeof...(Elements), std::true_type, std::false_type>::type type;
    };

    template<size_t idx, typename ... Elements>
    struct __is_last_parameter_helper {
      typedef typename std::conditional<idx + 1 == sizeof...(Elements) - 1, std::true_type, std::false_type>::type type;
    };

  }

  template<typename ... Elements>
  struct is_single_parameter_pack: public std::integral_constant<bool,
      __is_single_parameter_pack_helper<Elements...>::type::value> {
  };

  template<size_t idx, typename ... Elements>
  struct is_last_parameter: public std::integral_constant<bool, __is_last_parameter_helper<Elements...>::type::value> {
  };

} // atlas

#endif /* TYPE_TRAITS_H_ */

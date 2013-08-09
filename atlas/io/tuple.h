/*
 * tuple.h
 *
 *  Created on: Aug 9, 2013
 *      Author: vincent
 */

#ifndef ATLAS_IO_TUPLE_H_
#define ATLAS_IO_TUPLE_H_

#include <tuple>
#include <atlas/type_traits.h>

namespace atlas {
  namespace {

    const static char default_delimiter = ' ';

    template<size_t idx, typename ... Elements>
    void aux_put(std::ostream& os, std::tuple<Elements...>& t, char delimiter, single_parameter_pack_tag) {
      os << std::get<idx>(t);
    }

    template<size_t idx, typename ... Elements>
    void aux_put(std::ostream& os, std::tuple<Elements...>& t, char delimiter, not_single_parameter_pack_tag) {
      os << std::get<idx>(t) << delimiter;

      aux_put<idx + 1>(os, t, delimiter, atlas::is_last_parameter<idx, Elements...>());
    }

    template<typename ... Elements>
    void put(std::ostream& os, std::tuple<Elements...>& t, char delimiter, last_parameter_tag) {
      os << std::get<0>(t);
    }

    template<typename ... Elements>
    void put(std::ostream& os, std::tuple<Elements...>& t, char delimiter, not_last_parameter_tag) {
      aux_put<0>(os, t, delimiter, std::false_type());
    }
  }

  template<typename ... Elements>
  void print(std::ostream& os, std::tuple<Elements...>& t, char delimiter = default_delimiter) {
    atlas::put(os, t, delimiter, atlas::is_single_parameter_pack<Elements...>());
  }

} // atlas

namespace std {

  template<typename ... Elements>
  ostream& operator<<(ostream& os, tuple<Elements...>& t) {
    atlas::print(os, t);

    return os;
  }
}

#endif /* TUPLE_H_ */

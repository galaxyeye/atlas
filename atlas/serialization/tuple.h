/*
 * tuple.h
 *
 *  Created on: Jan 29, 2013
 *      Author: vincent
 */

#ifndef ATLAS_TUPLE_H_
#define ATLAS_TUPLE_H_

#include <tuple>
#include <atlas/type_traits.h>

namespace atlas {
  namespace {

    template<size_t idx, typename Archive, typename ... Elements>
    void aux_serialize(Archive& ar, std::tuple<Elements...>& t, single_parameter_pack_tag) {
      ar & std::get<idx>(t);
    }

    template<size_t idx, typename Archive, typename ... Elements>
    void aux_serialize(Archive& ar, std::tuple<Elements...>& t, not_single_parameter_pack_tag) {
      ar & std::get<idx>(t);

      aux_serialize<idx + 1>(ar, t, atlas::is_last_parameter<idx, Elements...>());
    }

    template<typename Archive, typename ... Elements>
    void serialize(Archive& ar, std::tuple<Elements...>& t, last_parameter_tag) {
      ar & std::get<0>(t);
    }

    template<typename Archive, typename ... Elements>
    void serialize(Archive& ar, std::tuple<Elements...>& t, not_last_parameter_tag) {
      aux_serialize<0>(ar, t, std::false_type());
    }
  }

} // atlas

namespace boost {
  namespace serialization {

    template<typename Archive, typename ... Elements>
    Archive& serialize(Archive& ar, std::tuple<Elements...>& t, const unsigned int version) {
      atlas::serialize(ar, t, atlas::is_single_parameter_pack<Elements...>());

      return ar;
    }

  } // serialization
} // boost

#endif /* TUPLE_H_ */

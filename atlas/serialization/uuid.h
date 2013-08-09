/*
 * uuid.h
 *
 *  Created on: Aug 8, 2013
 *      Author: vincent
 */

#ifndef ATLAS_SERIALIZATION_UUID_H_
#define ATLAS_SERIALIZATION_UUID_H_

#include <boost/uuid/uuid.hpp>

namespace boost {
  namespace serialization {

    template<typename Archive>
    void serialize(Archive& ar, boost::uuids::uuid& uuid, const unsigned int version) {
      ar & uuid.data;
    }

  } // serialization
} // boost

#endif /* ATLAS_SERIALIZATION_UUID_H_ */

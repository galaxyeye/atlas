/*
 * tuple.cpp
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#include <iostream>
#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <atlas/serialization/tuple.h>
#include <atlas/io/tuple.h>

int main() {
  {
    std::ofstream ofs("tuple.ar");
    boost::archive::text_oarchive oa(ofs);

    std::tuple<int, int, int> ints{1, 2, 3};
    oa << ints;
  }

  {
    std::ifstream ifs("tuple.ar");
    boost::archive::text_iarchive ia(ifs);

    std::tuple<int, int, int> ints;
    ia >> ints;

    std::cout << ints;
  }
}

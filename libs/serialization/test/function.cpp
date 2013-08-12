/*
 * function.cpp
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#include <string>
#include <iostream>
#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

#include <atlas/serialization/function.h>
#include <atlas/serialization/uuid.h>

using boost::uuids::uuid;
using boost::uuids::random_generator;
using boost::uuids::nil_uuid;
using atlas::serialization::function;

int add(int i, int j) {
  return i + j;
}

class C {
public:
  static int add(int i, int j) {
    return i + j;
  }
};

class U {
public:
  static uuid gen(const uuid& u) {
    return random_generator()();
  }
};

class S {
public:
  static std::string str(const std::string& s) {
    std::string str(s);
    return str.append("hello world");
  }
};

int main() {
  {
    std::ofstream ofs("add.rfc");
    boost::archive::text_oarchive oa(ofs);

    function<int(int, int)> serializable_add(add, 1, 2, oa);
  }

  {
    std::ifstream ifs("add.rfc");
    boost::archive::text_iarchive ia(ifs);

    function<int(int, int)> serializable_add(add, ia);
    std::cout << serializable_add() << std::endl;
  }

  {
    std::ofstream ofs("cadd.rfc");
    boost::archive::text_oarchive oa(ofs);

    function<int(int, int)> serializable_cadd(C::add, 10, 11, oa);
  }

  {
    std::ifstream ifs("cadd.rfc");
    boost::archive::text_iarchive ia(ifs);

    function<int(int, int)> serializable_cadd(C::add, ia);

    std::function<int()> f = std::bind(serializable_cadd);
    std::cout << f() << std::endl;
  }

  {
    std::ofstream ofs("str.rfc");
    boost::archive::text_oarchive oa(ofs);

    function<std::string(const std::string&)> fun(S::str, "12341312341234", oa);
  }

  {
    std::ifstream ifs("str.rfc");
    boost::archive::text_iarchive ia(ifs);

    function<std::string(const std::string&)> fun(S::str, ia);

    std::cout << fun() << std::endl;
  }

  {
    std::ofstream ofs("ugen.rfc");
    boost::archive::text_oarchive oa(ofs);

    function<uuid(const uuid&)> ugen(U::gen, nil_uuid(), oa);
  }

  {
    std::ifstream ifs("ugen.rfc");
    boost::archive::text_iarchive ia(ifs);

    function<uuid(const uuid&)> ugen(U::gen, ia);
    std::cout << ugen() << std::endl;
  }

  return 0;
}

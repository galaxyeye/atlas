/*
 * apply_tuple.cpp
 *
 *  Created on: Aug 9, 2013
 *      Author: vincent
 */

#include <iostream>
#include <functional>
#include <tuple>

#include <atlas/apply_tuple.h>

int add2(int i, int j) {
  return i + j;
}

int add3(int i, int j, int k) {
  return i + j + k;
}

class C {
public:

  int add2(int i, int j) {
    return i + j;
  }

  int add3(int i, int j, int k) {
    return i + j + k;
  }
};

int main() {
  // call free functions
  std::cout << atlas::apply_tuple(add2, std::make_tuple(1, 2)) << std::endl;
  std::cout << atlas::apply_tuple(add3, std::make_tuple(1, 2, 3)) << std::endl;

  // call member functions
//  C c;
//  auto cadd2 = std::bind(&C::add2, &c, std::placeholders::_1, std::placeholders::_2);
//  auto cadd3 = std::bind(&C::add2, &c, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

  // lol failed to compile, need a fix
//  std::cout << atlas::apply_tuple(cadd2, std::make_tuple(1, 2)) << std::endl;
//  std::cout << atlas::apply_tuple(cadd3, std::make_tuple(1, 2, 3)) << std::endl;
}

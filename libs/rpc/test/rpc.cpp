/*
 * rpc.cpp
 *
 *  Created on: Apr 16, 2013
 *      Author: vincent
 */

#include <iostream>
#include <string>
#include <functional>
#include <tuple>

#include <boost/mpl/int.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/pair.hpp>

#include <boost/fusion/container/generation/make_map.hpp>
#include <boost/fusion/include/make_map.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/make_tuple.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/io.hpp>

// #include <atlas/rfc/callable.h>

// using namespace atlas::rpc;
using namespace boost::fusion;

int f(int x, int y) { return x + y; }
void f2(int, int, const std::string&) {}

struct A {
public:
  void f(int x, int y) { std::cout << x * y; }
};

template<typename CallVec>
struct executor {

  executor(CallVec& calls) : calls(calls) {}

  template<int id, typename F>
  void apply(F&& f) {
    boost::fusion::at_c<id>(calls) = f;
  }

  template<int id, typename... Args>
  void execute(Args&&... args) {
    boost::fusion::at_c<id>(calls).second(std::forward<Args>(args)...);
  }

  CallVec& calls;
};

int main() {
  typedef boost::mpl::int_<0> _0;
  typedef boost::mpl::int_<1> _1;
  typedef boost::mpl::int_<2> _2;

  auto vec = make_vector(&f, &f2, std::mem_fn(&A::f));
  // auto idx = make_map<decltype(&f), decltype(&f2), decltype(std::mem_fn(&A::f)), _0ul, _1ul, _2ul>(0, 1, 2);
  // auto idx = make_map<decltype(&f), decltype(&f2), _0ul, _1ul>(_0ul(), _1ul());
  auto idx = make_map<int(*)(int, int), void(int, int, const std::string&), _0, _1>(_0(), _1());

  typedef typename result_of::at_key<decltype(idx), decltype(&f)>::type I;
//  static_assert(std::is_same<_0, std::decay<I>::type>::value, "same type");
  at_c<std::decay<I>::type::value>(vec)(1, 2);
//  // std::cout << at_key<decltype(&f)>(idx) << std::endl;
//  std::cout << _0::value;
//  at_c<0>(vec)(1, 2);
//
//  std::cout << typeid(at_c<0>(vec)).name() << std::endl;
//
//  std::cout << make_vector(123, "Hello", 'x') << std::endl;
//
//  std::cout << make_tuple(123, "Hello", 'x') << std::endl;

//  typedef map<pair<int, char>, pair<double, std::string> > map_type;
//
//  map_type m(make_pair<int>('X'), make_pair<double>("Men"));
//
//  auto m2 = make_map<int, double, char, std::string>('X', "Men");
//
//  std::cout << at_key<int>(m) << std::endl;
//  std::cout << at_key<double>(m) << std::endl;
//
//  std::cout << at_key<int>(m2) << std::endl;
//  std::cout << at_key<double>(m2) << std::endl;

//  A a;
//  auto vec = make_vector(&f, &f2, std::mem_fn(&A::f));
//  auto map = make_map<int, decltype(&f), int, decltype(&f2)>(0, &f, 1, &f2);
//  at_key<int>(map)(1, 2);

  // auto m = make_map(0, 1, 2, 2, 1, 0);

  // auto m = make_map<int, double>('X', "Men");

  // executor<decltype(map)> executor(map);
  // executor.execute<0>(1, 2);
//  executor.execute<1>(1, 2, "");
//  executor.execute<2>(a, 1, 2);
}

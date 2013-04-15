/*
 * singleton.cpp
 *
 *  Created on: Apr 15, 2013
 *      Author: vincent
 */

#include <iostream>
#include <vector>
#include <thread>

#include <atlas/singleton.h>

using namespace atlas;

struct A {

  A(int j) { std::cout << "A" << std::endl; i = j; ++i; }

  ~A() { std::cout << "~A" << std::endl; }

  void run() { std::cout << i << std::endl; }

  static int i;
};

int A::i;

void f(std::shared_ptr<A> p) {
  p->run();
}

void test() {
  singleton<A>::ref(5).run();
  singleton<A>::ref(0).run(); // a drawback, the arguments for construction is useless
  singleton<A>::ref(0).run();

  auto p = singleton<A>::ptr(4);
  p->run();
  auto p2 = p;
  p2->run();

  f(p);
}

int main() {
  static const size_t thread_num = 30;

  std::vector<std::shared_ptr<std::thread>> vt(thread_num);
  for (size_t i = 0; i < thread_num; ++i) vt.push_back(std::make_shared<std::thread>(test));
  for (size_t i = 0; i < vt.size(); ++i) vt[i]->join();
}

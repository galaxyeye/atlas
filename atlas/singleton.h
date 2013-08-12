/*
 * singleton.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent ivincent.zhang@gmail.com
 */

#ifndef ATLAS_SINGLETON_H_
#define ATLAS_SINGLETON_H_

#include <memory>
#include <mutex>

namespace atlas {

  // A thread safe singleton in C++11,
  // the managed object can be constructed with arguments.
  // The managed object can be used by reference or a shared_ptr.
  template<typename T>
  class singleton {
  public:

    ~singleton() = default;

    static T& ref() { return *ptr(); }

    static auto ptr() -> std::shared_ptr<T> {
      std::call_once(_only_one, __init);
      return _value;
    }

  private:

    static void __init() {
      _value = std::make_shared<T>();
    }

  private:

    static std::shared_ptr<T> _value;
    static std::once_flag _only_one;
  };

  template<typename T> std::shared_ptr<T> singleton<T>::_value;
  template<typename T> std::once_flag singleton<T>::_only_one;

} // atlas

#endif /* SINGLETON_H_ */

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

    template<typename... Args>
    static T& ref(Args&&... args) { return *ptr(std::forward<Args>(args)...); }

    template<typename... Args>
    static auto ptr(Args&&... args) -> std::shared_ptr<T> {
      std::call_once(_only_one, __init<Args...>, std::forward<Args>(args)...);
      return _value.lock();
    }

  private:

    template<typename... Args>
    static void __init(Args&&... args) {
      _value = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
    }

  private:

    static std::weak_ptr<T> _value;
    static std::once_flag _only_one;
  };

  template<typename T> std::weak_ptr<T> singleton<T>::_value;
  template<typename T> std::once_flag singleton<T>::_only_one;

} // atlas

#endif /* SINGLETON_H_ */

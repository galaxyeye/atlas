/*
 * tl_singleton.h
 *
 *  Created on: 2012-4-10
 *      Author: frank
 */

#ifndef THREAD_LOCAL_SINGLETON_H_
#define THREAD_LOCAL_SINGLETON_H_

namespace atlas {

    template<typename T>
    thread_local class tl_singleton {
    public:

      ~tl_singleton() = default;

      template<typename... Args>
      static T& ref(Args&&... args) {
        return *ptr(std::forward<Args>(args)...);
      }

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

#endif

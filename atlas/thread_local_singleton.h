/*
 * thread_local_singleton.h
 *
 *  Created on: 2012-4-10
 *      Author: frank
 */

#ifndef THREAD_LOCAL_SINGLETON_H_
#define THREAD_LOCAL_SINGLETON_H_

namespace atlas {
  namespace utility {

    template<typename T>
    class thread_local_singleton {
    public:

      static T& instance() {
        if (!_value) {
          _value = new T();
        }
        return *_value;
      }

      // no way to auto delete it
      static void destroy() {
        delete _value;
        _value = 0;
      }

    private:

      thread_local_singleton& operator=(const thread_local_singleton&) = delete;  // Disallow copying
      thread_local_singleton(const thread_local_singleton&) = delete;

    private:

      static __thread T* _value;
    };

    template<typename T>
    __thread T* thread_local_singleton<T>::_value = 0;

  } // utility
} // atlas

#endif

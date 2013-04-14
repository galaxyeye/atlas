/*
 * singleton.h
 *
 *  Created on: Apr 8, 2013
 *      Author: vincent
 */

// see http://silviuardelean.ro/2012/06/05/few-singleton-approaches/

#ifndef ATLAS_SINGLETON_H_
#define ATLAS_SINGLETON_H_

#include <memory>
#include <mutex>

namespace atlas {

  class singleton {
  public:

    ~singleton();

    static std::shared_ptr<singleton>& instance() {
      static std::shared_ptr<singleton> ins = _val.lock();

      if (!ins) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!ins) {
          ins.reset(new singleton());
          _val = instance;
        }
      }

      return instance;
    }

  private:

    static std::mutex _mutex;
    static std::weak_ptr<singleton> _val;

    singleton() {}
    singleton(const singleton& rs) = delete;
    singleton& operator=(const singleton& rs) = delete;
  };

  std::weak_ptr<singleton> singleton::_val = nullptr;
}

#endif /* SINGLETON_H_ */

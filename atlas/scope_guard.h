/*
 * scope_guard.h
 *
 *  Created on: May 1, 2013
 *      Author: vincent
 */

#ifndef ATLAS_SCOPE_GUARD_H_
#define ATLAS_SCOPE_GUARD_H_

#include <functional>

namespace atlas {

  class scope_guard {
  public:

    scope_guard(std::function<void()> fn) : _fn(fn) {}

    void dismiss() { _fn = nullptr; }

    ~scope_guard() noexcept {
      if (_fn) _fn();
    }

  private:

    std::function<void()> _fn;
  };

} // atlas

#endif /* ATLAS_SCOPE_GUARD_H_ */

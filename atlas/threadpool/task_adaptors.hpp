/*! \file
 * \brief Task adaptors.
 *
 * This file contains adaptors for task function objects.
 *
 * Copyright (c) 2005-2007 Philipp Henkel
 *
 * Use, modification, and distribution are  subject to the
 * Boost Software License, Version 1.0. (See accompanying  file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * http://threadpool.sourceforge.net
 *
 */

#ifndef THREADPOOL_TASK_ADAPTERS_HPP_INCLUDED
#define THREADPOOL_TASK_ADAPTERS_HPP_INCLUDED

#include <memory>
#include <functional>
#include <thread>

namespace boostplus {
  namespace threadpool {

    /*! \brief Standard task function object.
     *
     * This function object wraps a nullary function which returns void.
     *
     */
    typedef std::function<void()> task_func;

    /*! \brief Prioritized task function object.
     *
     * This function object wraps a task_func object and binds a priority to it.
     * prio_task_funcs can be compared using the operator < which realises a partial ordering.
     * The wrapped task function is invoked by calling the operator ().
     *
     * \see prio_scheduler
     *
     */
    class prio_task_func {
    public:

      typedef void result_type; //!< Indicates the functor's result type.

    public:

      prio_task_func(unsigned int priority, const task_func& function) :
          _priority(priority), _function(function) {
      }

      /*! Executes the task function.
       */
      void operator()(void) const {
        if (_function) {
          _function();
        }
      }

      /*! Comparison operator which realises a partial ordering based on priorities.
       * \param rhs The object to compare with.
       * \return true if the priority of *this is less than right hand side's priority, false otherwise.
       */
      bool operator<(const prio_task_func& rhs) const {
        return _priority < rhs._priority;
      }

    private:

      unsigned int _priority;
      task_func _function;
    };

    /*! \brief Looped task function object.
     *
     * This function object wraps a boolean thread function object.
     * The wrapped task function is invoked by calling the operator () and it is executed in regular
     * time intervals until false is returned. The interval length may be zero.
     * Please note that a pool's thread is engaged as long as the task is looped.
     *
     */
    class looped_task_func {
    private:

      std::function<bool()> _function; //!< The task's function.
      unsigned int _break_s; //!< Duration of breaks in seconds.
      unsigned int _break_ns; //!< Duration of breaks in nano seconds.

    public:

      typedef void result_type; //!< Indicates the functor's result type.

    public:

      /*! Constructor.
       * \param function The task's function object which is looped until false is returned.
       * \param interval The minimum break time in milli seconds before the first execution of the task function and between the following ones.
       */
      looped_task_func(const std::function<bool()>& function, const unsigned int interval = 0) :
          _function(function) {
        _break_s = interval / 1000;
        _break_ns = (interval - _break_s * 1000) * 1000 * 1000;
      }

      /*! Executes the task function.
       */
      void operator()() const {
        if (_function) {
          if (_break_s > 0 || _break_ns > 0) { // Sleep some time before first execution
//            xtime xt;
//            xtime_get(&xt, TIME_UTC);
//            xt.nsec += _break_ns;
//            xt.sec += _break_s;
            // std::thread::sleep_for(xt);

            // std::chrono::nanoseconds duration(_break_ns);
            // std::this_thread::sleep_for(duration);

          }

          while (_function()) {
            if (_break_s > 0 || _break_ns > 0) {
//              xtime xt;
//              xtime_get(&xt, TIME_UTC);
//              xt.nsec += _break_ns;
//              xt.sec += _break_s;
//              thread::sleep(xt);

              // std::chrono::nanoseconds duration(_break_ns);
              // std::this_thread::sleep_for(duration);
            }
            else {
              // std::this_thread::yield(); // Be fair to other threads
            }
          } // while
        } // if
      } // operator
    };
  } // threadpool
} // boostplus

#endif // THREADPOOL_TASK_ADAPTERS_HPP_INCLUDED

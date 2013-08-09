/*! \file
 * \brief Thread pool core.
 *
 * This file contains the threadpool's core class: pool<Task, SchedulingPolicy>.
 *
 * Thread pools are a mechanism for asynchronous and parallel processing
 * within the same process. The pool class provides a convenient way
 * for dispatching asynchronous tasks as functions objects. The scheduling
 * of these tasks can be easily controlled by using customized schedulers.
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

#ifndef THREADPOOL_POOL_CORE_HPP_INCLUDED
#define THREADPOOL_POOL_CORE_HPP_INCLUDED

#include "locking_ptr.hpp"
#include "worker_thread.hpp"

#include "../task_adaptors.hpp"

#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

/// The namespace threadpool contains a thread pool and related utility classes.
namespace boostpp {
  namespace threadpool {
    namespace detail {

      /*! \brief Thread pool.
       *
       * Thread pools are a mechanism for asynchronous and parallel processing
       * within the same process. The pool class provides a convenient way
       * for dispatching asynchronous tasks as functions objects. The scheduling
       * of these tasks can be easily controlled by using customized schedulers.
       * A task must not throw an exception.
       *
       * A pool_impl is DefaultConstructible and NonCopyable.
       *
       * \param Task A function object which implements the operator 'void operator() (void) const'. The operator () is called by the pool to execute the task. Exceptions are ignored.
       * \param Scheduler A task container which determines how tasks are scheduled. It is guaranteed that this container is accessed only by one thread at a time. The scheduler shall not throw exceptions.
       *
       * \remarks The pool class is thread-safe.
       *
       * \see Tasks: task_func, prio_task_func
       * \see Scheduling policies: fifo_scheduler, lifo_scheduler, prio_scheduler
       */
      template<typename Task, template<typename > class SchedulingPolicy, template<typename > class SizePolicy,
          template<typename > class SizePolicyController, template<typename > class ShutdownPolicy>
      class pool_core : public std::enable_shared_from_this<
          pool_core<Task, SchedulingPolicy, SizePolicy, SizePolicyController, ShutdownPolicy>>,
          private boost::noncopyable {
      public:

        // Type definitions
        typedef Task task_type;                                 //!< Indicates the task's type.
        typedef SchedulingPolicy<task_type> scheduler_type;     //!< Indicates the scheduler's type.
        typedef pool_core<Task, SchedulingPolicy, SizePolicy, SizePolicyController, ShutdownPolicy> pool_type; //!< Indicates the thread pool's type.

        typedef SizePolicy<pool_type> size_policy_type;         //!< Indicates the sizer's type.
        // typedef typename size_policy_type::size_controller size_controller_type;

        typedef SizePolicyController<pool_type> size_controller_type;

        // typedef SizePolicy<pool_type>::size_controller size_controller_type;
        typedef ShutdownPolicy<pool_type> shutdown_policy_type;    //!< Indicates the shutdown policy's type.

        typedef worker_thread<pool_type> worker_type;

        // The task is required to be a nullary function.
        // BOOST_STATIC_ASSERT(std::function_traits<task_type()>::arity == 0);

        // The task function's result type is required to be void.
        static_assert(std::is_void<typename std::result_of<task_type()>::type >::value, "must be void");

      private:
        // Friends
        friend class worker_thread<pool_type> ;

        friend class SizePolicy<pool_type> ;
        friend class ShutdownPolicy<pool_type> ;

      private:

        // The following members may be accessed by _multiple_ threads at the same time:
        volatile size_t _worker_count;
        volatile size_t _target_worker_count;
        volatile size_t _active_worker_count;

      private:

        // The following members are accessed only by _one_ thread at the same time:
        scheduler_type _scheduler;
        std::unique_ptr<size_policy_type> _size_policy; // is never null

        bool _terminate_all_workers;								// Indicates if termination of all workers was triggered.
        std::vector<std::shared_ptr<worker_type> > _terminated_workers; // List of workers which are terminated but not fully destructed.

      private:

        // The following members are implemented thread-safe:
        mutable std::recursive_mutex _monitor;
        mutable std::condition_variable _worker_idle_or_terminated_event;	// A worker is idle or was terminated.
        mutable std::condition_variable _task_or_terminate_workers_event; // Task is available OR total worker count should be reduced.

      public:

        /// Constructor.
        pool_core() : _worker_count(0), _target_worker_count(0), _active_worker_count(0), _terminate_all_workers(false) {
          pool_type volatile & self_ref = *this;
          _size_policy.reset(new size_policy_type(self_ref));

          _scheduler.clear();
        }

        /// Destructor.
        ~pool_core() { }

        /*! Gets the size controller which manages the number of threads in the pool.
         * \return The size controller.
         * \see SizePolicy
         */
        size_controller_type size_controller() {
          return size_controller_type(*_size_policy, this->shared_fro_this());
        }

        /*! Gets the number of threads in the pool.
         * \return The number of threads.
         */
        size_t size() const volatile {
          return _worker_count;
        }

        // TODO is only called once
        void shutdown() {
          ShutdownPolicy<pool_type>::shutdown(*this);
        }

        /*! Schedules a task for asynchronous execution. The task will be executed once only.
         * \param task The task function object. It should not throw execeptions.
         * \return true, if the task could be scheduled and false otherwise.
         */
        bool schedule(task_type const & task) volatile {
          locking_ptr<pool_type, std::recursive_mutex> lockedThis(*this, _monitor);

          if (lockedThis->_scheduler.push(task)) {
            lockedThis->_task_or_terminate_workers_event.notify_one();
            return true;
          }
          else {
            return false;
          }
        }

        /*! Returns the number of tasks which are currently executed.
         * \return The number of active tasks.
         */
        size_t active() const volatile {
          return _active_worker_count;
        }

        /*! Returns the number of tasks which are ready for execution.
         * \return The number of pending tasks.
         */
        size_t pending() const volatile {
          locking_ptr<const pool_type, std::recursive_mutex> lockedThis(*this, _monitor);
          return lockedThis->_scheduler.size();
        }

        /*! Removes all pending tasks from the pool's scheduler.
         */
        void clear() volatile {
          locking_ptr<pool_type, std::recursive_mutex> lockedThis(*this, _monitor);
          lockedThis->_scheduler.clear();
        }

        /*! Indicates that there are no tasks pending.
         * \return true if there are no tasks ready for execution.
         * \remarks This function is more efficient that the check 'pending() == 0'.
         */
        bool empty() const volatile {
          locking_ptr<const pool_type, std::recursive_mutex> lockedThis(*this, _monitor);
          return lockedThis->_scheduler.empty();
        }

        /*! The current thread of execution is blocked until the sum of all active
         *  and pending tasks is equal or less than a given threshold.
         * \param task_threshold The maximum number of tasks in pool and scheduler.
         */
        void wait(size_t const task_threshold = 0) const volatile {
          const pool_type* self = const_cast<const pool_type*>(this);
          std::lock_guard<std::recursive_mutex> lock(self->_monitor);

          if (0 == task_threshold) {
            while (0 != self->_active_worker_count || !self->_scheduler.empty()) {
              self->_worker_idle_or_terminated_event.wait(lock);
            }
          }
          else {
            while (task_threshold < self->_active_worker_count + self->_scheduler.size()) {
              self->_worker_idle_or_terminated_event.wait(lock);
            }
          }
        }

        /*! The current thread of execution is blocked until the timestamp is met
         * or the sum of all active and pending tasks is equal or less
         * than a given threshold.
         * \param timestamp The time when function returns at the latest.
         * \param task_threshold The maximum number of tasks in pool and scheduler.
         * \return true if the task sum is equal or less than the threshold, false otherwise.
         */
        bool wait(xtime const & timestamp, size_t const task_threshold = 0) const volatile {
          const pool_type* self = const_cast<const pool_type*>(this);
          std::lock_guard<std::recursive_mutex> lock(self->_monitor);

          if (0 == task_threshold) {
            while (0 != self->_active_worker_count || !self->_scheduler.empty()) {
              if (!self->_worker_idle_or_terminated_event.timed_wait(lock, timestamp)) return false;
            }
          }
          else {
            while (task_threshold < self->_active_worker_count + self->_scheduler.size()) {
              if (!self->_worker_idle_or_terminated_event.timed_wait(lock, timestamp)) return false;
            }
          }

          return true;
        }

      private:

        void terminate_all_workers(bool const wait) volatile {
          pool_type* self = const_cast<pool_type*>(this);
          std::lock_guard<std::recursive_mutex> lock(self->_monitor);

          self->_terminate_all_workers = true;

          _target_worker_count = 0;
          self->_task_or_terminate_workers_event.notify_all();

          if (wait) {
            while (_active_worker_count > 0) {
              self->_worker_idle_or_terminated_event.wait(lock);
            }

            for (auto it = self->_terminated_workers.begin(); it != self->_terminated_workers.end(); ++it) {
              (*it)->join();
            }

            self->_terminated_workers.clear();
          }
        }

        /*! Changes the number of worker threads in the pool. The resizing
         *  is handled by the SizePolicy.
         * \param threads The new number of worker threads.
         * \return true, if pool will be resized and false if not.
         */
        bool resize(size_t const worker_count) volatile {
          locking_ptr<pool_type, std::recursive_mutex> lockedThis(*this, _monitor);

          if (!_terminate_all_workers) {
            _target_worker_count = worker_count;
          }
          else {
            return false;
          }

          if (_worker_count <= _target_worker_count) { // increase worker count
            while (_worker_count < _target_worker_count) {
              try {
                worker_thread<pool_type>::create_and_attach(lockedThis->shared_fro_this());
                _worker_count++;
                _active_worker_count++;
              }
              catch (...) {
                // TODO : catch what?
                return false;
              }
            }
          }
          else { // decrease worker count
            lockedThis->_task_or_terminate_workers_event.notify_all();   // TODO: Optimize number of notified workers
          }

          return true;
        }

        // worker died with unhandled exception
        void worker_died_unexpectedly(std::shared_ptr<worker_type> worker) volatile {
          locking_ptr<pool_type, std::recursive_mutex> lockedThis(*this, _monitor);

          _worker_count--;
          _active_worker_count--;
          lockedThis->_worker_idle_or_terminated_event.notify_all();

          if (_terminate_all_workers) {
            lockedThis->_terminated_workers.push_back(worker);
          }
          else {
            lockedThis->_size_policy->worker_died_unexpectedly(_worker_count);
          }
        }

        void worker_destructed(std::shared_ptr<worker_type> worker) volatile {
          locking_ptr<pool_type, std::recursive_mutex> lockedThis(*this, _monitor);
          _worker_count--;
          _active_worker_count--;
          lockedThis->_worker_idle_or_terminated_event.notify_all();

          if (_terminate_all_workers) {
            lockedThis->_terminated_workers.push_back(worker);
          }
        }

        bool execute_task() volatile {
          std::function<void()> task;

          { // fetch task
            pool_type* lockedThis = const_cast<pool_type*>(this);
            std::lock_guard<std::mutex> lock(lockedThis->_monitor);

            // decrease number of threads if necessary
            if (_worker_count > _target_worker_count) {
              return false;	// terminate worker
            }

            // wait for tasks
            while (lockedThis->_scheduler.empty()) {
              // decrease number of workers if necessary
              if (_worker_count > _target_worker_count) {
                return false;	// terminate worker
              }
              else {
                _active_worker_count--;
                lockedThis->_worker_idle_or_terminated_event.notify_all();
                lockedThis->_task_or_terminate_workers_event.wait(lock);
                _active_worker_count++;
              }
            }

            task = lockedThis->_scheduler.top();
            lockedThis->_scheduler.pop();
          }

          // call task function
          if (task) {
            task();
          }

          //guard->disable();
          return true;
        }
      };

    } // detail
  } // threadpool
} // boostpp

#endif // THREADPOOL_POOL_CORE_HPP_INCLUDED

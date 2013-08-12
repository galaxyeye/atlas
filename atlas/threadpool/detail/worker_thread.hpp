/*! \file
 * \brief Thread pool worker.
 *
 * The worker thread instance is attached to a pool
 * and executes tasks of this pool.
 *
 * Copyright (c) 2005-2007 Philipp Henkel
 *
 * Use, modification, and distribution are  subject to the
 * boostplus Software License, Version 1.0. (See accompanying  file
 * LICENSE_1_0.txt or copy at http://www.boostplus.org/LICENSE_1_0.txt)
 *
 * http://threadpool.sourceforge.net
 *
 */

#ifndef THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED
#define THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED

#include <cassert>
#include <memory>
#include <thread>
#include <mutex>

namespace boostplus {
  namespace threadpool {
    namespace detail {

      class scope_guard {
      public:

        scope_guard(std::function<void()> fn) : _fn(fn) {}

        void disable() { _fn = nullptr; }

        ~scope_guard() noexcept {
          if (_fn) _fn();
        }

      private:

        std::function<void()> _fn;
      };

      /*! \brief Thread pool worker.
       *
       * A worker_thread represents a thread of execution. The worker is attached to a
       * thread pool and processes tasks of that pool. The lifetime of the worker and its
       * internal boostplus::thread is managed automatically.
       *
       * This class is a helper class and cannot be constructed or accessed directly.
       *
       * \see pool_core
       */
      template<typename Pool>
      class worker_thread : public std::enable_shared_from_this<worker_thread<Pool>>
      {
      public:

        typedef Pool pool_type;

        worker_thread() = default;
        worker_thread(worker_thread&) = delete;
        worker_thread(const worker_thread&) = delete;
        worker_thread& operator=(const worker_thread&) = delete;

      private:

        /*! Constructs a new worker.
         * \param pool Pointer to it's parent pool.
         * \see function create_and_attach
         */
        worker_thread(const std::shared_ptr<pool_type>& pool) : _pool(pool) {
        }

        /*! Notifies that an exception occurred in the run loop.
         */
        void died_unexpectedly() {
          _pool->worker_died_unexpectedly(this->shared_from_this());
        }

      public:

        /*! Executes pool's tasks sequentially.
         */
        void run() {
          scope_guard notify_exception(std::bind(&worker_thread::died_unexpectedly, this));

          while (_pool->execute_task()) {}

          notify_exception.disable();
          _pool->worker_destructed(this->shared_from_this());
        }

        /*! Joins the worker's thread.
         */
        void join() {
          _thread->join();
        }

        /*! Constructs a new worker thread and attaches it to the pool.
         * \param pool Pointer to the pool.
         */
        static void create_and_attach(const std::shared_ptr<pool_type>& pool) {
          std::shared_ptr<worker_thread<Pool>> worker(new worker_thread(pool));

          if (worker) {
            worker->_thread.reset(new std::thread(std::bind(&worker_thread::run, worker)));
          }
        }

      private:

        std::shared_ptr<pool_type> _pool;
        std::shared_ptr<std::thread> _thread;
      };
    } // detail
  } // threadpool
} // boostplus

#endif // THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED

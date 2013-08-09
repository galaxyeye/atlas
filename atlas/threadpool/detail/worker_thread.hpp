/*! \file
 * \brief Thread pool worker.
 *
 * The worker thread instance is attached to a pool
 * and executes tasks of this pool.
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

#ifndef THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED
#define THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED

#include <atlas/scope_guard.h>

namespace boostpp {
  namespace threadpool {
    namespace detail {

      /*! \brief Thread pool worker.
       *
       * A worker_thread represents a thread of execution. The worker is attached to a
       * thread pool and processes tasks of that pool. The lifetime of the worker and its
       * internal std::thread is managed automatically.
       *
       * This class is a helper class and cannot be constructed or accessed directly.
       *
       * \see pool_core
       */
      template<typename Pool>
      class worker_thread : public std::enable_shared_from_this<worker_thread<Pool> >, private boost::noncopyable {
      public:

        typedef Pool pool_type;         	   //!< Indicates the pool's type.

      private:
        std::shared_ptr<pool_type> _pool;     //!< Pointer to the pool which created the worker.
        std::shared_ptr<std::thread> _thread;   //!< Pointer to the thread which executes the run loop.

        /*! Constructs a new worker.
         * \param pool Pointer to it's parent pool.
         * \see function create_and_attach
         */
        worker_thread(std::shared_ptr<pool_type> const & pool) : _pool(pool) {
          assert(pool);
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
          atlas::scope_guard notify_exception([this]() { died_unexpectedly(); });

          while (_pool->execute_task()) {
          }

          notify_exception.dismiss();
          _pool->worker_destructed(this->shared_from_this());
        }

        /*! Joins the worker's thread.
         */
        void join() { _thread->join(); }

        /*! Constructs a new worker thread and attaches it to the pool.
         * \param pool Pointer to the pool.
         */
        static void create_and_attach(std::shared_ptr<pool_type> const & pool) {
          std::shared_ptr<worker_thread> worker(new worker_thread(pool));
          if (worker) {
            worker->_thread.reset(new std::thread(bind(&worker_thread::run, worker)));
          }
        }

      };

    } // detail
  } // threadpool
} // boost

#endif // THREADPOOL_DETAIL_WORKER_THREAD_HPP_INCLUDED

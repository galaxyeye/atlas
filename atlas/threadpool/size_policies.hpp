/*! \file
 * \brief Size policies.
 *
 * This file contains size policies for thread_pool. A size
 * policy controls the number of worker threads in the pool.
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

#ifndef THREADPOOL_SIZE_POLICIES_HPP_INCLUDED
#define THREADPOOL_SIZE_POLICIES_HPP_INCLUDED

#include <memory>
#include <functional>

namespace boostplus {
  namespace threadpool {

    /*! \brief SizePolicyController which provides no functionality.
     *
     * \param Pool The pool's core type.
     */
    template<typename Pool>
    struct empty_controller {
      empty_controller(typename Pool::size_policy_type&, std::shared_ptr<Pool>) {}
    };

    /*! \brief SizePolicyController which allows resizing.
     *
     * \param Pool The pool's core type.
     */
    template<typename Pool>
    class resize_controller {
    public:

      typedef typename Pool::size_policy_type size_policy_type;

    public:

      resize_controller(size_policy_type& policy, std::shared_ptr<Pool> pool) :
          _policy(policy), _pool(pool) {
      }

      bool resize(size_t worker_count) {
        return _policy.get().resize(worker_count);
      }

    private:

      std::reference_wrapper<size_policy_type> _policy;
      std::shared_ptr<Pool> _pool; //!< to make sure that the pool is alive (the policy pointer is valid) as long as the controller exists
    };

    /*! \brief SizePolicy which preserves the thread count.
     *
     * \param Pool The pool's core type.
     */
    template<typename Pool>
    class static_size {
    public:

      static void init(Pool& pool, size_t worker_count) {
        pool.resize(worker_count);
      }

      static_size(Pool& pool) : _pool(pool) {
      }

      bool resize(size_t worker_count) {
        return _pool.get().resize(worker_count);
      }

      void worker_died_unexpectedly(size_t new_worker_count) {
        _pool.get().resize(new_worker_count + 1);
      }

      // TODO this functions are not called yet
      void task_scheduled() {
      }

      void task_finished() {
      }

    private:

      std::reference_wrapper<Pool> _pool;
    };
  } // threadpool
} // boostplus

#endif // THREADPOOL_SIZE_POLICIES_HPP_INCLUDED

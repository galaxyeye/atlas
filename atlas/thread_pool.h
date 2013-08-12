/*
 * thread_pool.h
 *
 *  Created on: Aug 9, 2013
 *      Author: vincent
 */

#ifndef ATLAS_THREAD_POOL_H_
#define ATLAS_THREAD_POOL_H_

#include <atlas/threadpool/pool.hpp>
#include <atlas/threadpool/pool_adaptors.hpp>
#include <atlas/threadpool/task_adaptors.hpp>

namespace atlas {

  typedef boostplus::threadpool::fifo_pool fifo_thread_pool;
  typedef boostplus::threadpool::lifo_pool lifo_thread_pool;
  typedef boostplus::threadpool::prio_pool prio_thread_pool;

} // atlas

#endif /* ATLAS_THREAD_POOL_H_ */

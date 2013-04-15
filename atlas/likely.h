/*
 * likely.h
 *
 *  Created on: Apr 14, 2013
 *      Author: vincent
 */

#ifndef ATLAS_LIKELY_H_
#define ATLAS_LIKELY_H_

#undef likely
#undef unlikely

#define likely(x)   (__builtin_expect((x), 1))
#define unlikely(x) (__builtin_expect((x), 0))

#endif /* LIKELY_H_ */

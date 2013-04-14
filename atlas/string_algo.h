/*
 * algorithm.h
 *
 *  Created on: Apr 5, 2013
 *      Author: vincent
 */

#ifndef ATLAS_ALGORITHM_H_
#define ATLAS_ALGORITHM_H_

#include <string>
#include <type_traits>
#include <algorithm>

namespace atlas {

  namespace {
    struct useless {};

    template<typename UnsignedLong>
    int compare_unsigned_long(UnsignedLong n, UnsignedLong n2) {
      const static size_t min_int = std::numeric_limits<int>::min();
      const static size_t max_int = std::numeric_limits<int>::max();

      const size_t d = size_t(n - n2);

      if (d > max_int) return std::numeric_limits<int>::max();
      else if (d < min_int) return std::numeric_limits<int>::min();
      else return int(d);
    }
  }

  template<typename T>
  int compare(T n, T n2) {
    return n < n2;
  }

  template<>
  int compare(unsigned long n, unsigned long n2) {
    return compare_unsigned_long(n, n2);
  }

  template<>
  int compare(unsigned long long n, unsigned long long n2) {
    return compare_unsigned_long(n, n2);
  }

  /**
   *  @brief  Compare a character %array against another.
   *  @param s  character %array.
   *  @param n  Number of characters of s.
   *  @param s2  character %array to compare against.
   *  @param n2  Number of characters of s2.
   *  @return  Integer < 0, 0, or > 0.
   *
   *  NB: s must have at least n characters, s2 must have at least
   *  n2 characters, &apos;\\0&apos; has no special meaning.
  */
  template<typename Ch, typename Traits = std::char_traits<Ch>>
  int compare_unchecked(const Ch* s, size_t n, const Ch* s2, size_t n2) {
    const size_t len = std::min(n, n2);

    int r = Traits::compare(s, s2, len);
    if (!r) r = compare(n, n2);

    return r;
  }

  // When n = 1 way faster than the general multichar
  // Traits::copy/move/assign.
  template<typename Ch, typename Traits = std::char_traits<Ch>>
  void fast_copy(Ch* dest, const Ch* src, size_t n) {
    if (n == 1) Traits::assign(*dest, *src);
    else Traits::copy(dest, src, n);
  }

  template<typename Ch, typename Traits = std::char_traits<Ch>>
  void fast_copy_backward(Ch* dest, const Ch* src, size_t n) {
    if (n == 1) Traits::assign(*dest, *src);
    else std::copy_backward(src, src + n, dest);
  }

  template<typename Ch, typename Traits = std::char_traits<Ch>>
  void fast_move(Ch* dest, const Ch* src, size_t n) {
    if (n == 1) Traits::assign(*dest, *src);
    else Traits::move(dest, src, n);
  }

  template<typename Ch, typename Traits = std::char_traits<Ch>>
  void fast_assign(Ch* dest, size_t n, Ch c) {
    if (n == 1) Traits::assign(*dest, c);
    else Traits::assign(dest, n, c);
  }

} // atlas

#endif /* ATLAS_ALGORITHM_H_ */

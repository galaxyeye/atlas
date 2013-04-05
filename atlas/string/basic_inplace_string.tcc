/*
 * basic_inplace_string.h
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#ifndef ATLAS_BASIC_INPLACE_STRING_TCC_
#define ATLAS_BASIC_INPLACE_STRING_TCC_

#include <cassert>

namespace atlas {

  template<typename T, size_t N, typename Traits>
  template<size_t N2>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const basic_inplace_string<T, N2, Traits>& str) :
      _size(std::min(str.size(), capacity()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<size_t N2>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const basic_inplace_string<T, N2, Traits>& str, size_type pos, size_type n) :
      _size(__size_limit(0, str.__pos_limit(pos, n)))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<size_t N2>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const basic_inplace_string<T, N2, Traits>& str) :
      _size(__size_limit(0, str.size()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<typename Alloc>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const std::basic_string<T, Traits, Alloc>& str) :
      _size(__size_limit(0, str.size()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<typename Alloc>
  basic_inplace_string<T, N, Traits>::
  basic_inplace_string(const std::basic_string<T, Traits, Alloc>& str, size_type pos, size_type n) :
      _size(__size_limit(0, str.size()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(std::initializer_list<T> l) {
    for (auto v : l) {
      if (__free_size()) _data[_size++] = v;
    }

    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<class InputIterator>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(InputIterator first, InputIterator last) {
    // TODO : optimization when it's RandomAccessIterator
    while (first != last && __free_size()) {
      _data[_size++] = *first++;
    }

    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  void basic_inplace_string<T, N, Traits>::resize(size_type n, T c) {
    if (n <= size()) {
      __set_size(n);
    }
    else {
      n = __size_limit(0, n);
      __set_size(n);
      std::fill_n(end(), n - size(), c);
    }

    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  void basic_inplace_string<T, N, Traits>::fill(const T& u) {
    __set_size(capacity());
    std::fill_n(begin(), _size, u);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  void basic_inplace_string<T, N, Traits>::swap(basic_inplace_string& other) {
    size_type n = _size < other._size ? _size : other._size;
    while (n--) {
      std::swap(_data[n], other._data[n]);
    }

    std::swap(_size, other._size);
    __terminate();
    other.__terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<size_t N2>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::append(const basic_inplace_string<T, N2, Traits>& str) {
    __greedy_clone(str.data(), 0, str.size());

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  template<size_t N2>
  basic_inplace_string&
  basic_inplace_string<T, N, Traits>::append(const basic_inplace_string<T, N2, Traits>& str, size_type pos, size_type n) {
    if (pos > str.size()) throw std::out_of_range("basic_inplace_string::append");

    __greedy_clone(str.data(), pos, str.__pos_limit(pos, n));

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  template<typename Alloc>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::
  append(const std::basic_string<T, Traits, Alloc>& str, size_type pos, size_type n = npos) {
    if (pos > str.size()) throw std::out_of_range("basic_inplace_string::append");

    __greedy_clone(str.data(), pos, std::min(str.size() - pos, n));

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::append(const T* str, size_type n) {
    __greedy_clone(str, 0, n);

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::append(size_type n, T c) {
    const size_type append_size = __size_limit(size(), n);

    if (append_size) {
      __set_size(append_size);
      std::fill_n(begin(), append_size, c);
      __terminate();
    }

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  template<class InputIterator>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::append(InputIterator first, InputIterator last) {
    size_type n = __free_size();

    while (first != last && n--) {
      _data[_size++] = *first++;
    }
    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::copy(T* s, size_type n, size_type pos) const {
    __check(pos, "basic_string::copy");
    n = __pos_limit(pos, n);
    if (n) __fast_copy(s, data() + pos, n);

    // do not append null, see std::string::copy
    return n;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::insert(size_type pos, const T* s, size_type n) {
    __check(pos, "basic_string::insert");
    __check_size(size_type(0), n, "basic_string::insert");

    const_iterator middle = cbegin() + pos;
    const_iterator sbegin = s;
    const_iterator send = s + n;

    // TODO : __fast_copy/__fast_copy_backward version to faster
    std::copy_backward(middle, end(), end() + n);
    if (send < middle) {
      std::copy(sbegin, send, middle);
    }
    else if (sbegin < middle && send > middle) {
      std::copy_backward(sbegin, send, middle + n);
    }
    else {
      std::copy(sbegin + n, send + n, middle);
    }

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  template<typename Alloc>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::
  insert(size_type pos, const std::basic_string<T, Traits, Alloc>& str, size_type pos2, size_type n = npos) {
    if (pos2 > str.size()) throw std::out_of_range("basic_inplace_string::insert");

    n = __size_limit(pos, std::min(str.size() - pos, n));
    return insert(pos, str.data() + pos2, n);
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::erase(size_type pos = 0, size_type n = npos) {
    __check(pos, "basic_inplace_string::erase");
    n = __pos_limit(pos, n);

    __set_size(size() - n);
    __mutate_unckecked(pos, n, size_type(0));
    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::iterator
  basic_inplace_string<T, N, Traits>::erase(iterator first, iterator last) {
    const size_type size = last - first;

    if (size) {
      const size_type pos = first - begin();
      __mutate_unckecked(pos, size, size_type(0));
      return iterator(data() + pos);
    }
    else return first;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string&
  basic_inplace_string<T, N, Traits>::replace(size_type pos, size_type n, const T* str, size_type n2) {
    __check(pos, "basic_string::replace");
    n = __pos_limit(pos, n);
    n2 = __size_limit(size() - n, n2);

    if (__disjunct(str)) return __replace_unchecked(pos, n, str, n2);

    bool left;
    if ((left = (str + n2 <= data() + pos)) || data() + pos + n <= str) {
      __set_size(size() - n + n2);

      // Work in-place: non-overlapping case.
      size_type off = str - data();
      left ? off : (off += n2 - n);
      __mutate_unckecked(pos, n, n2);
      __fast_copy(data() + pos, data() + off, n2);

      __terminate();
      return *this;
    }
    else {
      // Todo: overlapping case.
      const basic_inplace_string tmp(str, n2);
      return __replace_unchecked(pos, n, tmp.data(), n2);
    }
  }

  template<typename T, size_t N, typename Traits>
  template<typename Alloc>
  basic_inplace_string&
  basic_inplace_string<T, N, Traits>::
  replace(size_type pos, size_type n, const std::basic_string<T, Traits, Alloc>& str, size_type pos2,
      size_type n2 = npos) {
    if (pos2 > str.size()) throw std::out_of_range("basic_inplace_string::insert");

    n2 = __size_limit(pos, std::min(str.size() - pos + n, n2));
    return replace(pos, n, str.data() + str.__check(pos2, "basic_inplace_string::replace"), n2);
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::find(const T* s, size_type pos, size_type n) const {
    const size_type size = size();
    const T* d = data();

    if (n == 0) return pos <= size ? pos : npos;

    if (n <= size) {
      for (; pos <= size - n; ++pos) {
        if (traits_type::eq(d[pos], s[0]) && traits_type::compare(d + pos + 1, s + 1, n - 1) == 0) {
          return pos;
        }
      } // for
    } // if

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::find(T c, size_type pos = 0) const {
    size_type ret = npos;
    const size_type size = size();

    if (pos < size) {
      const T* d = data();
      const size_type n = size - pos;
      const T* p = traits_type::find(d + pos, n, c);

      if (p) ret = p - d;
    }

    return ret;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::rfind(const T* s, size_type pos, size_type n) const {
    const size_type sz = size();

    if (n <= sz) {
      pos = std::min(size_type(sz - n), pos);

      do {
        if (traits_type::compare(data() + pos, s, n) == 0) return pos;
      } while (pos--);
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::rfind(T c, size_type pos) const {
    size_type sz = size();

    if (sz) {
      if (--sz > pos) sz = pos;
      for (++sz; sz-- > 0; ) if (traits_type::eq(_data[sz], c)) return sz;
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::find_first_of(const T* s, size_type pos, size_type n) const {
    for (; n && pos < size(); ++pos) {
      const T* p = traits_type::find(s, n, _data[pos]);
      if (p) return pos;
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::find_first_not_of(const T* s, size_type pos, size_type n) const {
    for (; pos < size(); ++pos) {
      if (!traits_type::find(s, n, _data[pos])) return pos;
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::find_first_not_of(T c, size_type pos) const {
    for (; pos < size(); ++pos) {
      if (!traits_type::eq(_data[pos], c)) return pos;
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::
  find_last_not_of(const T* s, size_type pos, size_type n) const {
    size_type sz = size();

    if (sz) {
      if (--sz > pos) sz = pos;

      do {
        if (!traits_type::find(s, n, _data[sz])) return sz;
      } while (sz--);
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::size_type
  basic_inplace_string<T, N, Traits>::
  find_last_not_of(T c, size_type pos) const {
    size_type sz = size();

    if (sz) {
      if (--sz > pos) sz = pos;

      do {
        if (!traits_type::eq(_data[sz], c)) return sz;
      } while (sz--);
    }

    return npos;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>&
  basic_inplace_string<T, N, Traits>::__replace_unchecked(size_type pos, size_type n, const T* src, size_type n2) {
    __set_size(size() - n + n2);

    __mutate_unckecked(pos, n, n2); // move range [pos + n, pos + n2) to [pos + inc_size, size + inc_size)
    if (n2) __fast_copy(data() + pos, src, n2); // fill blank range [pos, pos + n)

    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>&
  basic_inplace_string<T, N, Traits>::__replace_unchecked(size_type pos, size_type n, size_type n2, T c) {
    __set_size(size() - n + n2);

    __mutate_unckecked(pos, n, n2); // move range [pos + n, pos + n2) to [pos + inc_size, size + inc_size)
    if (n2) __fast_assign(data() + pos, n2, c); // fill blank range [pos, pos + n)

    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  void basic_inplace_string<T, N, Traits>::__mutate_unckecked(size_type pos, size_type len, size_type len2) {
    const size_type old_size = size();
    const size_type new_size = old_size + len2 - len;
    const size_type move_size = old_size - pos - len;

    std::assert(new_size < capacity());

    if (move_size && len != len2) __fast_move(data() + pos + len2, data() + pos + len, move_size);
  }

  template<typename T, size_t N, typename Traits>
  int basic_inplace_string<T, N, Traits>::
  __compare_unchecked(const T* s, size_type n, const T* s2, size_type n2) const {
    const size_type len = std::min(n, n2);

    int r = traits_type::compare(s, s2, len);
    if (!r) r = __compare(n, n2);

    return r;
  }

  template<typename T, size_t N, typename Traits>
  void basic_inplace_string<T, N, Traits>::__greedy_clone(const T* src, size_type pos, size_type n) {
    const size_type append_size = __size_limit(size(), n);

    if (append_size) {
      __set_size(size() + append_size);
      __fast_copy(_data, src, _size);
      __terminate();
    }
  }

} // atlas

#endif  /* ATLAS_BASIC_INPLACE_STRING_TCC_ */

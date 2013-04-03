/*
 * basic_inplace_string.h
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#ifndef ATLAS_BASIC_INPLACE_STRING_TCC_
#define ATLAS_BASIC_INPLACE_STRING_TCC_

namespace atlas {

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const basic_inplace_string& str) :
      _size(std::min(str.size(), capacity()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const basic_inplace_string& str, size_type pos, size_type n) :
      _size(__size_limit(0, str.__pos_limit(pos, n)))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const basic_inplace_string& str) :
      _size(__size_limit(0, str.size()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<typename Ch, typename ChTraits, typename Alloc>
  basic_inplace_string<T, N, Traits>::basic_inplace_string(const std::basic_string<Ch, ChTraits, Alloc>& str) :
      _size(__size_limit(0, str.size()))
  {
    __fast_copy(_data, str.data(), _size);
    __terminate();
  }

  template<typename T, size_t N, typename Traits>
  template<typename Ch, typename ChTraits, typename Alloc>
  basic_inplace_string<T, N, Traits>::
  basic_inplace_string(const std::basic_string<Ch, ChTraits, Alloc>& str, size_type pos, size_type n) :
      _size(__size_limit(0, str.size()))
  {
    __fast_copy(_data, str.data(), _size);
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
  basic_inplace_string& basic_inplace_string<T, N, Traits>::append(const basic_inplace_string& str) {
    __greedy_clone(str.data(), 0, str.size());

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string&
  basic_inplace_string<T, N, Traits>::append(const basic_inplace_string& str, size_type pos, size_type n) {
    if (pos > str.size()) throw std::out_of_range("basic_inplace_string::append");

    __greedy_clone(str.data(), pos, str.__pos_limit(pos, n));

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  template<typename Ch, typename ChTraits, typename Alloc>
  basic_inplace_string& basic_inplace_string<T, N, Traits>::
  append(const std::basic_string<Ch, ChTraits, Alloc>& str, size_type pos, size_type n = npos) {
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
  basic_inplace_string& basic_inplace_string<T, N, Traits>::erase(size_type pos = 0, size_type n = npos) {
    __check(pos, "basic_inplace_string::erase");
    n = __pos_limit(pos, n);

    __set_size(size() - n);
    __mutate(pos, n, size_type(0));
    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>::iterator
  basic_inplace_string<T, N, Traits>::erase(iterator first, iterator last) {
    const size_type size = last - first;

    if (size) {
      const size_type pos = first - begin();
      __mutate(pos, size, size_type(0));
      return iterator(data() + pos);
    }
    else return first;
  }

  // TODO : check the correctness again
  template<typename T, size_t N, typename Traits>
  basic_inplace_string&
  basic_inplace_string<T, N, Traits>::replace(size_type pos, size_type n, const T* str, size_type n2) {
    __check(pos, "basic_string::replace");
    n = __pos_limit(pos, n);
    __check_size(n, n2, "basic_string::replace");

    bool left;
    if (__disjunct(str)) return __replace_unchecked(pos, n, str, n2);
    else if ((left = str + n2 <= data() + pos) || data() + pos + n <= str) {
      // Work in-place: non-overlapping case.
      size_type off = str - data();
      left ? off : (off += n2 - n);
      __mutate(pos, n, n2);
      __fast_copy(data() + pos, data() + off, n2);
      return *this;
    }
    else {
      // Todo: overlapping case.
      const basic_inplace_string tmp(str, n2);
      return __replace_unchecked(pos, n, tmp.data(), n2);
    }
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>&
  basic_inplace_string<T, N, Traits>::__replace_unchecked(size_type pos, size_type n, const T* src, size_type n2) {
    __set_size(n2);

    __mutate(pos, n, n2);
    if (n2) __fast_copy(data() + pos, src, n2);

    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  basic_inplace_string<T, N, Traits>&
  basic_inplace_string<T, N, Traits>::__replace_unchecked(size_type pos, size_type n, size_type n2, T c) {
    // __check_size(n, n2, "basic_string::__replace_aux");

    __set_size(n2);
    __mutate(pos, n, n2);
    if (n2) __fast_assign(data() + pos, n2, c);
    __terminate();

    return *this;
  }

  template<typename T, size_t N, typename Traits>
  void basic_inplace_string<T, N, Traits>::__mutate(size_type pos, size_type len, size_type len2) {
    const size_type old_size = size();
    const size_type new_size = old_size + len2 - len;
    const size_type how_much = old_size - pos - len;

    if (new_size > capacity()) throw std::out_of_range("__mutate");
    else if (how_much && len != len2) __fast_move(data() + pos + len2, data() + pos + len, how_much);
  }

} // atlas

#endif  /* ATLAS_BASIC_INPLACE_STRING_TCC_ */

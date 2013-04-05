/*
 * basic_inplace_string.h
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#ifndef ATLAS_BASIC_INPLACE_STRING_H_
#define ATLAS_BASIC_INPLACE_STRING_H_

#include <initializer_list>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <limits>

namespace atlas {

  /**
   *  @brief A standard compatible container for storing a fixed max size string
   *
   *  Provide both std::array like and std::string like interface
   *
   *  @param  T  Type of element. Required to be a complete type.
   *  @param  N  Number of elements.
   */
  template<typename T, std::size_t N, typename Traits>
  class basic_inplace_string {
  public:

    typedef Traits traits_type;
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  public:

    // Construct/copy/destroy:

    /**
     *  @brief  Default constructor creates an empty string.
     */
    basic_inplace_string() = default;

    /**
     *  @brief  Construct string with copy of value of @a str whose capacity is smaller.
     *  Prevent to construct a string using a bigger string, should use other constructors instead.
     *  @param  str  Source string.
     */
    template<size_t N2>
    basic_inplace_string(const basic_inplace_string<T, N2, Traits>& str);

    template<typename Alloc>
    basic_inplace_string(const std::basic_string<T, Traits, Alloc>& str);

    /**
     *  @brief  Construct string as copy of a substring. The string might be truncated.
     *  @param  str  Source string.
     *  @param  pos  Index of first character to copy from.
     *  @param  n  Number of characters to copy (default remainder).
     */
    template<size_t N2>
    basic_inplace_string(const basic_inplace_string<T, N2, Traits>& str, size_type pos, size_type n = npos);

    template<typename Alloc>
    basic_inplace_string(const std::basic_string<T, Traits, Alloc>& str, size_type pos, size_type n = npos);

    /**
     *  @brief  Construct string initialized by a character %array.. The string might be truncated.
     *  @param  str  Source character %array.
     *  @param  n  Number of characters to copy.
     *
     *  NB: @a str must have at least @a n characters, &apos;\\0&apos;
     *  has no special meaning.
     */
    basic_inplace_string(const T* str, size_type n) {
#ifdef __ATLAS_PERFORMANCE_TEST__
      n = __size_limit(0, n);
      while (*str && n--) _data[_size++] = *str++;
      __terminate();
#else
      // how fast traits_type::length could be?
      __greedy_clone(str, 0, __size_limit(0, std::min(n, traits_type::length(str))));
#endif
    }

    /**
     *  @brief  Construct string as copy of a C string. The string might be truncated.
     *  @param  str  Source C string.
     */
    basic_inplace_string(const T* str) {
#ifdef __ATLAS_PERFORMANCE_TEST__
      while (*str && size() <= capacity()) {
        _data[_size++] = *str++;
      }

      __terminate();
#else
      __greedy_clone(str, 0, __size_limit(0, traits_type::length(str)));
#endif
    }

    /**
     *  @brief  Construct string as multiple characters.
     *  @param  n  Number of characters.
     *  @param  c  Character to use.
     */
    basic_inplace_string(size_type n, T c) {
      _size = __size_limit(0, n);
      std::fill_n(_data, _size, c);
      __terminate();
    }

    /**
     *  @brief  No move constructor
     **/
    template<size_t N2>
    basic_inplace_string(basic_inplace_string<T, N2, Traits>&&) = delete;

    /**
     *  @brief  Construct string from an initializer %list. The string might be truncated.
     *  @param  l  std::initializer_list of characters.
     */
    basic_inplace_string(std::initializer_list<T> l);

    /**
     *  @brief  Construct string as copy of a range. The string might be truncated.
     *  @param  first  Start of range.
     *  @param  last  End of range.
     */
    template<class InputIterator>
    basic_inplace_string(InputIterator first, InputIterator last);

    ~basic_inplace_string() = default;

    /**
     *  @brief  Assign the value of @a str to this string.
     *  Prevent to construct a string using a bigger string, should use other constructors instead.
     *  @param  str  Source string.
     */
    template<size_t N2>
    basic_inplace_string& operator=(const basic_inplace_string<T, N2, Traits>& str)
    { return assign(str); }

    template<typename Alloc>
    basic_inplace_string& operator=(const std::basic_string<T, Traits, Alloc>& str, size_type pos, size_type n = npos)
    { return assign(str); }

    /**
     *  @brief  Copy contents of @a s into this string. The string might be truncated.
     *  @param  str  Source null-terminated string.
     */
    basic_inplace_string& operator=(const T* str) {
      return assign(str);
    }

    /**
     *  @brief  Set value to string of length 1. The string might be truncated.
     *  @param  c  Source character.
     *
     *  Assigning to a character makes this string length 1
     */
    basic_inplace_string& operator=(T c) {
      assign(1, c);
      return *this;
    }

    /**
     *  @brief  No move assignment
     **/
    template<size_t N2>
    basic_inplace_string& operator=(basic_inplace_string<T, N2, Traits>&& str) = delete;

    /**
     *  @brief  Set value to string constructed from initializer %list. The string might be truncated.
     *  @param  l  std::initializer_list.
     */
    basic_inplace_string& operator=(std::initializer_list<T> l) {
      assign(l.begin(), l.size());
      return *this;
    }

    // std::array::fill
    void fill(const T& u);

    /**
     *  @brief  Swap element one by one. The longer string will be truncated.
     *  TODO : check this API, should we delete it?
     **/
    void swap(basic_inplace_string& other);

    // Iterators.
    iterator begin() {return iterator(data()); }

    const_iterator begin() const {return const_iterator(data()); }

    iterator end() {return iterator(data() + size()); }

    const_iterator end() const {return const_iterator(data() + size()); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }

    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    const_iterator cbegin() const { return const_iterator(data()); }

    const_iterator cend() const { return const_iterator(data() + size()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }

    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    reference front() {return *begin();}

    const_reference front() const {return *begin();}

    reference back() {return *(end() - 1);}

    const_reference back() const {return *(end() - 1);}

    pointer data() {return std::__addressof(_data[0]);}

    const_pointer data() const {return std::__addressof(_data[0]);}

    const_pointer c_str() const {return data();}

    std::string str() const {return c_str();}

    constexpr size_type capacity() const { return N;}

    size_type size() const { return _size; }

    size_type length() const { return _size; }

    constexpr size_type max_size() const { return N; }

    /**
     *  @brief  Resizes the %string to the specified number of characters. The string may not longer than capacity
     *  @param  n  Number of characters the %string should contain.
     *  @param  c  Character to fill any new elements.
     *
     *  This function will %resize the %string to the specified
     *  number of characters.  If the number is smaller than the
     *  %string's current size the %string is truncated, otherwise
     *  the %string is extended and new elements are %set to @a c.
     *
     *  The total size of the new size should not greater than capacity
     */
    void resize(size_type n, T c);

    /**
     *  @brief  Resizes the %string to the specified number of characters.
     *  @param  n  Number of characters the %string should contain.
     *
     *  This function will resize the %string to the specified length.  If
     *  the new size is smaller than the %string's current size the %string
     *  is truncated, otherwise the %string is extended and new characters
     *  are default-constructed.  For basic types such as char, this means
     *  setting them to 0.
     */
    void resize(size_type n) {resize(n, T());}

    bool empty() const {return size() == 0;}

    // Element access.
    reference operator[](size_type n) { return _data[n]; }

    constexpr const_reference operator[](size_type n) const { return _data[n]; }

    reference at(size_type n) {
      if (n >= size()) throw std::out_of_range("basic_inplace_string::at");
      return _data[n];
    }

    constexpr const_reference at(size_type n) const {
      // Result of conditional expression must be an lvalue so use
      // boolean ? lvalue : (throw-expr, lvalue)
      return n < size() ? _data[n] : (throw std::out_of_range("basic_inplace_string::at"), _data[0]);
    }

    // Modifiers:
    /**
     *  @brief  Append a string to this string. May append a truncated string.
     *  @param str  The string to append.
     *  @return  Reference to this string.
     */
    basic_inplace_string& operator+=(const basic_inplace_string& str)
    { return append(str);}

    /**
     *  @brief  Append a C string. May append a truncated string.
     *  @param str  The C string to append.
     *  @return  Reference to this string.
     */
    basic_inplace_string& operator+=(const T* str)
    { return append(str);}

    /**
     *  @brief  Append a character.
     *  @param c  The character to append.
     *  @return  Reference to this string.
     */
    basic_inplace_string& operator+=(T c) {
      push_back(c);
      return *this;
    }

    /**
     *  @brief  Append an std::initializer_list of characters. May append a truncated string.
     *  @param l  The std::initializer_list of characters to be appended.
     *  @return  Reference to this string.
     */
    basic_inplace_string& operator+=(std::initializer_list<T> l)
    { return append(l.begin(), l.size());}

    /**
     *  @brief  Append a string to this string. May append a truncated string.
     *  @param str  The string to append.
     *  @return  Reference to this string.
     */
    template<size_t N2>
    basic_inplace_string& append(const basic_inplace_string<T, N2, Traits>& str);

    /**
     *  @brief  Append a substring. May append a truncated string.
     *  @param str  The string to append.
     *  @param pos  Index of the first character of str to append.
     *  @param n  The number of characters to append.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range if @a pos is not a valid index.
     *
     *  This function appends @a n characters from @a str
     *  starting at @a pos to this string.  If @a n is is larger
     *  than the number of available characters in @a str, the
     *  remainder of @a str is appended.
     */
    template<size_t N2>
    basic_inplace_string& append(const basic_inplace_string<T, N2, Traits>& str, size_type pos, size_type n = npos);

    /**
     *  @brief  Append a string to this string. May append a truncated string.
     *  @param str  The string to append.
     *  @return  Reference to this string.
     */
    template<typename Alloc>
    basic_inplace_string& append(const std::basic_string<T, Traits, Alloc>& str) {
      __greedy_clone(str.data(), 0, str.size());
      return *this;
    }

    /**
     *  @brief  Append a substring. May append a truncated string.
     *  @param str  The string to append.
     *  @param pos  Index of the first character of str to append.
     *  @param n  The number of characters to append.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range if @a pos is not a valid index.
     *
     *  This function appends @a n characters from @a str
     *  starting at @a pos to this string.  If @a n is is larger
     *  than the number of available characters in @a str, the
     *  remainder of @a str is appended.
     */
    template<typename Alloc>
    basic_inplace_string& append(const std::basic_string<T, Traits, Alloc>& str, size_type pos, size_type n = npos);

    /**
     *  @brief  Append a C substring. May append a truncated string.
     *  @param str  The C string to append.
     *  @param n  The number of characters to append.
     *  @return  Reference to this string.
     */
    basic_inplace_string& append(const T* str, size_type n);

    /**
     *  @brief  Append a C string. May append a truncated string.
     *  @param str  The C string to append.
     *  @return  Reference to this string.
     */
    basic_inplace_string& append(const T* str) {
      // TODO : performance with traits_type::length?
      return append(str, traits_type::length(str));
    }

    /**
     *  @brief  Append multiple characters. May not append all n characters.
     *  @param n  The number of characters to append.
     *  @param c  The character to use.
     *  @return  Reference to this string.
     *
     *  Appends n copies of c to this string.
     */
    basic_inplace_string& append(size_type n, T c);

    /**
     *  @brief  Append an std::initializer_list of characters. May append a truncated string.
     *  @param l  The std::initializer_list of characters to append.
     *  @return  Reference to this string.
     */
    basic_inplace_string& append(std::initializer_list<T> l)
    { return append(l.begin(), l.size());}

    /**
     *  @brief  Append a range of characters. May append a truncated string.
     *  @param first  Iterator referencing the first character to append.
     *  @param last  Iterator marking the end of the range.
     *  @return  Reference to this string.
     *
     *  Appends characters in the range [first,last) to this string.
     */
    template<class InputIterator>
    basic_inplace_string& append(InputIterator first, InputIterator last);

    /**
     *  @brief  Append a single character. No guarantee to success.
     *  @param c  Character to append.
     */
    void push_back(T c) {
      if (0 == __free_size()) return;

      _data[_size++] = c;
      __terminate();
    }

    /**
     *  @brief  Set value to contents of another string. May assign a truncated string.
     *  @param  str  Source string to use.
     *  @return  Reference to this string.
     */
    template<size_t N2>
    basic_inplace_string& assign(const basic_inplace_string<T, N2, Traits>& str) {
      return assign(str, 0, str.size());
    }

    /**
     * @brief No move assignment
     */
    template<size_t N2>
    basic_inplace_string& assign(basic_inplace_string<T, N2, Traits>&&) = delete;

    /**
     *  @brief  Set value to a substring of a string. May assign a truncated string.
     *  @param str  The string to use.
     *  @param pos  Index of the first character of str.
     *  @param n  Number of characters to use.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range if @a pos is not a valid index.
     *
     *  This function sets this string to the substring of @a str
     *  consisting of @a n characters at @a pos.  If @a n is
     *  is larger than the number of available characters in @a
     *  str, the remainder of @a str is used.
     */
    template<size_t N2>
    basic_inplace_string& assign(const basic_inplace_string<T, N2, Traits>& str, size_type pos, size_type n) {
      if (this == std::__addressof(str)) return *this;
      __reset();
      return append(str, pos, n);
    }

    template<typename Alloc>
    basic_inplace_string& assign(const std::basic_string<T, Traits, Alloc>& str) {
      __reset();
      return append(str);
    }

    /**
     *  @brief  Set value to a substring of a string. May assign a truncated string.
     *  @param str  The string to use.
     *  @param pos  Index of the first character of str.
     *  @param n  Number of characters to use.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range if @a pos is not a valid index.
     *
     *  This function sets this string to the substring of @a str
     *  consisting of @a n characters at @a pos.  If @a n is
     *  is larger than the number of available characters in @a
     *  str, the remainder of @a str is used.
     */
    template<typename Alloc>
    basic_inplace_string& assign(const std::basic_string<T, Traits, Alloc>& str, size_type pos, size_type n = npos) {
      __reset();
      return append(str, pos, n);
    }

    /**
     *  @brief  Set value to a C substring. May assign a truncated string.
     *  @param str  The C string to use.
     *  @param n  Number of characters to use.
     *  @return  Reference to this string.
     *
     *  This function sets the value of this string to the first @a n
     *  characters of @a str.  If @a n is is larger than the number of
     *  available characters in @a str, the remainder of @a str is used.
     */
    basic_inplace_string& assign(const T* str, size_type n) {
      __reset();
      return append(str, n);
    }

    /**
     *  @brief  Set value to contents of a C string. May assign a truncated string.
     *  @param str  The C string to use.
     *  @return  Reference to this string.
     *
     *  This function sets the value of this string to the value of @a str.
     *  The data is copied, so there is no dependence on @a str once the
     *  function returns.
     */
    basic_inplace_string& assign(const T* str) {
      // TODO : performance
      return assign(str, traits_type::length(str));
    }

    /**
     *  @brief  Set value to multiple characters. Do not guarantee all n characters are assigned
     *  @param n  Length of the resulting string.
     *  @param c  The character to use.
     *  @return  Reference to this string.
     *
     *  This function sets the value of this string to @a n copies of
     *  character @a c.
     */
    basic_inplace_string& assign(size_type n, T c) {
      __reset();
      return append(n, c);
    }

    /**
     *  @brief  Set value to a range of characters.
     *  @param first  Iterator referencing the first character to append.
     *  @param last  Iterator marking the end of the range.
     *  @return  Reference to this string.
     *
     *  Sets value of string to characters in the range [first,last).
     */
    template<class InputIterator>
    basic_inplace_string& assign(InputIterator first, InputIterator last)
    { return replace(begin(), end(), first, last);}

    /**
     *  @brief  Set value to an std::initializer_list of characters.
     *  @param l  The std::initializer_list of characters to assign.
     *  @return  Reference to this string.
     */
    basic_inplace_string& assign(std::initializer_list<T> l)
    { return assign(l.begin(), l.size());}

    /**
     *  @brief  Copy substring into C string.
     *  @param s  C string to copy value into.
     *  @param n  Number of characters to copy.
     *  @param pos  Index of first character to copy.
     *  @return  Number of characters actually copied
     *  @throw  std::out_of_range  If pos > size().
     *
     *  Copies up to @a n characters starting at @a pos into the
     *  C string @a s.  If @a pos is %greater than size(),
     *  out_of_range is thrown.
     */
    size_type copy(T* s, size_type n, size_type pos = 0) const;

    /**
     *  @brief  Insert multiple characters. Do not guarantee all n characters are inserted
     *  @param p  Iterator referencing location in string to insert at.
     *  @param n  Number of characters to insert
     *  @param c  The character to insert.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Inserts @a n copies of character @a c starting at the
     *  position referenced by iterator @a p.  If adding
     *  characters causes the length to exceed max_size(),
     *  length_error is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    void insert(iterator p, size_type n, T c)
    { replace(p, p, n, c);}

    /**
     *  @brief  Insert a range of characters. Do not guarantee all n characters are inserted
     *  @param p  Iterator referencing location in string to insert at.
     *  @param first  Start of range.
     *  @param last  End of range.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Inserts characters in range [first,last).  If adding
     *  characters causes the length to exceed max_size(),
     *  length_error is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    template<class InputIterator>
    void insert(iterator p, InputIterator first, InputIterator last)
    { replace(p, p, first, last);}

    /**
     *  @brief  Insert an std::initializer_list of characters. Do not guarantee all n characters are inserted
     *  @param p  Iterator referencing location in string to insert at.
     *  @param l  The std::initializer_list of characters to insert.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     */
    void insert(iterator p, std::initializer_list<T> l)
    { insert(p - begin(), l.begin(), l.size());}

    /**
     *  @brief  Insert value of a string. Do not guarantee all n characters are inserted
     *  @param pos1  Iterator referencing location in string to insert at.
     *  @param str  The string to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Inserts value of @a str starting at @a pos1.  If adding
     *  characters causes the length to exceed max_size(),
     *  length_error is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    template<size_t N2>
    basic_inplace_string& insert(size_type pos, const basic_inplace_string<T, N2, Traits>& str)
    { return insert(pos, str, size_type(0), str.size());}

    template<typename Alloc>
    basic_inplace_string& insert(size_type pos, const std::basic_string<T, Traits, Alloc>& str)
    { return insert(pos, str, size_type(0), str.size());}

    /**
     *  @brief  Insert a substring. Do not guarantee all n characters are inserted
     *  @param pos  Iterator referencing location in string to insert at.
     *  @param str  The string to insert.
     *  @param pos2  Start of characters in str to insert.
     *  @param n  Number of characters to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *  @throw  std::out_of_range  If @a pos1 > size() or
     *  @a pos2 > @a str.size().
     *
     *  Starting at @a pos1, insert @a n character of @a str
     *  beginning with @a pos2.  If adding characters causes the
     *  length to exceed max_size(), length_error is thrown.  If @a
     *  pos1 is beyond the end of this string or @a pos2 is
     *  beyond the end of @a str, out_of_range is thrown.  The
     *  value of the string doesn't change if an error is thrown.
     */
    template<size_t N2>
    basic_inplace_string& insert(size_type pos, const basic_inplace_string<T, N2, Traits>& str, size_type pos2, size_type n) {
      n = __size_limit(pos, str.__pos_limit(pos2, n));
      return insert(pos, str.data() + str.__check(pos2, "basic_inplace_string::insert"), n);
    }

    template<typename Alloc>
    basic_inplace_string&
    insert(size_type pos, const std::basic_string<T, Traits, Alloc>& str, size_type pos2, size_type n = npos);

    /**
     *  @brief  Insert a C substring. Do not guarantee all characters are inserted
     *  @param pos  Iterator referencing location in string to insert at.
     *  @param str  The C string to insert.
     *  @param n  The number of characters to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *  @throw  std::out_of_range  If @a pos is beyond the end of this
     *  string.
     *
     *  Inserts the first @a n characters of @a str starting at @a
     *  pos.  If adding characters causes the length to exceed
     *  max_size(), length_error is thrown.  If @a pos is beyond
     *  end(), out_of_range is thrown.  The value of the string
     *  doesn't change if an error is thrown.
     */
    basic_inplace_string& insert(size_type pos, const T* str, size_type n);

    /**
     *  @brief  Insert a C string. Do not guarantee all characters are inserted
     *  @param pos  Iterator referencing location in string to insert at.
     *  @param str  The C string to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *  @throw  std::out_of_range  If @a pos is beyond the end of this
     *  string.
     *
     *  Inserts the first @a n characters of @a str starting at @a pos.  If
     *  adding characters causes the length to exceed max_size(),
     *  length_error is thrown.  If @a pos is beyond end(), out_of_range is
     *  thrown.  The value of the string doesn't change if an error is
     *  thrown.
     */
    basic_inplace_string& insert(size_type pos, const T* str) {
      return insert(pos, str, traits_type::length(str));
    }

    /**
     *  @brief  Insert multiple characters. Do not guarantee all characters are inserted
     *  @param pos  Index in string to insert at.
     *  @param n  Number of characters to insert
     *  @param c  The character to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *  @throw  std::out_of_range  If @a pos is beyond the end of this
     *  string.
     *
     *  Inserts @a n copies of character @a c starting at index
     *  @a pos.  If adding characters causes the length to exceed
     *  max_size(), length_error is thrown.  If @a pos > length(),
     *  out_of_range is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    basic_inplace_string& insert(size_type pos, size_type n, T c) {
      return __replace_unchecked(__check(pos, "basic_inplace_string::insert"), size_type(0), __size_limit(size(), n), c);
    }

    /**
     *  @brief  Insert one character.
     *  @param p  Iterator referencing position in string to insert at.
     *  @param c  The character to insert.
     *  @return  Iterator referencing newly inserted char.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Inserts character @a c at position referenced by @a p.
     *  If adding character causes the length to exceed max_size(),
     *  length_error is thrown.  If @a p is beyond end of string,
     *  out_of_range is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    iterator insert(iterator p, T c) {
      const size_type pos = p - begin();
      __replace_unchecked(pos, size_type(0), size_type(1), c);
      return iterator(data() + pos);
    }

    /**
     *  @brief  Remove characters.
     *  @param pos  Index of first character to remove (default 0).
     *  @param n  Number of characters to remove (default remainder).
     *  @return  Reference to this string.
     *  @throw  std::out_of_range  If @a pos is beyond the end of this
     *  string.
     *
     *  Removes @a n characters from this string starting at @a
     *  pos.  The length of the string is reduced by @a n.  If
     *  there are < @a n characters to remove, the remainder of
     *  the string is truncated.  If @a p is beyond end of string,
     *  out_of_range is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    basic_inplace_string& erase(size_type pos = 0, size_type n = npos);

    /**
     *  @brief  Remove one character.
     *  @param position  Iterator referencing the character to remove.
     *  @return  iterator referencing same location after removal.
     *
     *  Removes the character at @a position from this string. The value
     *  of the string doesn't change if an error is thrown.
     */
    iterator erase(iterator it) {
      const size_type pos = std::distance(begin(), it);
      std::move(it, end(), --it); // move would be always better?
      return std::advance(begin(), pos);
    }

    /**
     *  @brief  Remove a range of characters.
     *  @param first  Iterator referencing the first character to remove.
     *  @param last  Iterator referencing the end of the range.
     *  @return  Iterator referencing location of first after removal.
     *
     *  Removes the characters in the range [first,last) from this string.
     *  The value of the string doesn't change if an error is thrown.
     */
    iterator erase(iterator first, iterator last);

    /**
     *  @brief  Remove the last character.
     *
     *  The string must be non-empty.
     */
    void pop_back() {erase(size() - 1, 1);}

    /**
     *  @brief  Replace characters with value from another string.
     *  @param pos  Index of first character to replace.
     *  @param n  Number of characters to be replaced.
     *  @param str  String to insert.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range  If @a pos is beyond the end of this
     *  string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [pos,pos+n) from
     *  this string.  In place, the value of @a str is inserted.
     *  If @a pos is beyond end of string, out_of_range is thrown.
     *  If the length of the result exceeds max_size(), length_error
     *  is thrown.  The value of the string doesn't change if an
     *  error is thrown.
     */
    template<size_t N2>
    basic_inplace_string& replace(size_type pos, size_type n, const basic_inplace_string<T, N2, Traits>& str)
    { return replace(pos, n, str.data(), str.size());}

    template<typename Alloc>
    basic_inplace_string& replace(size_type pos, const std::basic_string<T, Traits, Alloc>& str)
    { return replace(pos, str, size_type(0), str.size());}

    /**
     *  @brief  Replace characters with value from another string.
     *  @param pos1  Index of first character to replace.
     *  @param n  Number of characters to be replaced.
     *  @param str  String to insert.
     *  @param pos2  Index of first character of str to use.
     *  @param n2  Number of characters from str to use.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range  If @a pos1 > size() or @a pos2 >
     *  str.size().
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [pos1,pos1 + n) from this
     *  string.  In place, the value of @a str is inserted.  If @a pos is
     *  beyond end of string, out_of_range is thrown.  If the length of the
     *  result exceeds max_size(), length_error is thrown.  The value of the
     *  string doesn't change if an error is thrown.
     */
    template<size_t N2>
    basic_inplace_string&
    replace(size_type pos, size_type n, const basic_inplace_string<T, N2, Traits>& str, size_type pos2, size_type n2) {
      n2 = __size_limit(pos, str.__pos_limit(pos2, n2));
      return replace(pos, n, str.data() + str.__check(pos2, "basic_inplace_string::replace"), n2);
    }

    template<typename Alloc>
    basic_inplace_string&
    replace(size_type pos, size_type n, const std::basic_string<T, Traits, Alloc>& str, size_type pos2, size_type n2 = npos);

    /**
     *  @brief  Replace characters with value of a C substring.
     *  @param pos  Index of first character to replace.
     *  @param n1  Number of characters to be replaced.
     *  @param str  C string to insert.
     *  @param n2  Number of characters from @a s to use.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range  If @a pos1 > size().
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [pos,pos + n1)
     *  from this string.  In place, the first @a n2 characters of
     *  @a str are inserted, or all of @a str if @a n2 is too large.  If
     *  @a pos is beyond end of string, out_of_range is thrown.  If
     *  the length of result exceeds max_size(), length_error is
     *  thrown.  The value of the string doesn't change if an error
     *  is thrown.
     */
    basic_inplace_string& replace(size_type pos, size_type n, const T* str, size_type n2);

    /**
     *  @brief  Replace characters with value of a C string.
     *  @param pos  Index of first character to replace.
     *  @param n1  Number of characters to be replaced.
     *  @param str  C string to insert.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range  If @a pos > size().
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [pos,pos + n1)
     *  from this string.  In place, the characters of @a str are
     *  inserted.  If @a pos is beyond end of string, out_of_range
     *  is thrown.  If the length of result exceeds max_size(),
     *  length_error is thrown.  The value of the string doesn't
     *  change if an error is thrown.
     */
    basic_inplace_string& replace(size_type pos, size_type n1, const T* str) {
      return replace(pos, n1, str, traits_type::length(str));
    }

    /**
     *  @brief  Replace characters with multiple characters.
     *  @param pos  Index of first character to replace.
     *  @param n1  Number of characters to be replaced.
     *  @param n2  Number of characters to insert.
     *  @param c  Character to insert.
     *  @return  Reference to this string.
     *  @throw  std::out_of_range  If @a pos > size().
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [pos,pos + n1) from this
     *  string.  In place, @a n2 copies of @a c are inserted.
     *  If @a pos is beyond end of string, out_of_range is thrown.
     *  If the length of result exceeds max_size(), length_error is
     *  thrown.  The value of the string doesn't change if an error
     *  is thrown.
     */
    basic_inplace_string& replace(size_type pos, size_type n1, size_type n2, T c) {
      return __replace_unchecked(__check(pos, "basic_inplace_string::replace"), __pos_limit(pos, n1), n2, c);
    }

    /**
     *  @brief  Replace range of characters with string.
     *  @param first  Iterator referencing start of range to replace.
     *  @param last  Iterator referencing end of range to replace.
     *  @param str  String value to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [first,last).  In place,
     *  the value of @a str is inserted.  If the length of result
     *  exceeds max_size(), length_error is thrown.  The value of
     *  the string doesn't change if an error is thrown.
     */
    template<size_t N2>
    basic_inplace_string& replace(iterator first, iterator last, const basic_inplace_string<T, N2, Traits>& str)
    { return replace(first, last, str.data(), str.size());}

    /**
     *  @brief  Replace range of characters with C substring.
     *  @param first  Iterator referencing start of range to replace.
     *  @param last  Iterator referencing end of range to replace.
     *  @param str  C string value to insert.
     *  @param n  Number of characters from s to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [first,last).  In place,
     *  the first @a n characters of @a str are inserted.  If the
     *  length of result exceeds max_size(), length_error is thrown.
     *  The value of the string doesn't change if an error is
     *  thrown.
     */
    basic_inplace_string& replace(iterator first, iterator last, const T* str, size_type n) {
      return replace(first - begin(), last - first, str, n);
    }

    /**
     *  @brief  Replace range of characters with C string.
     *  @param first  Iterator referencing start of range to replace.
     *  @param last  Iterator referencing end of range to replace.
     *  @param str  C string value to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [first,last).  In place,
     *  the characters of @a str are inserted.  If the length of
     *  result exceeds max_size(), length_error is thrown.  The
     *  value of the string doesn't change if an error is thrown.
     */
    basic_inplace_string& replace(iterator first, iterator last, const T* str) {
      return replace(first, last, str, traits_type::length(str));
    }

    /**
     *  @brief  Replace range of characters with multiple characters
     *  @param first  Iterator referencing start of range to replace.
     *  @param last  Iterator referencing end of range to replace.
     *  @param n  Number of characters to insert.
     *  @param c  Character to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [first,last).  In place,
     *  @a n copies of @a c are inserted.  If the length of
     *  result exceeds max_size(), length_error is thrown.  The
     *  value of the string doesn't change if an error is thrown.
     */
    basic_inplace_string& replace(iterator first, iterator last, size_type n, T c) {
      const size_type pos = first - begin();
      return __replace_unchecked(pos, last - first, __pos_limit(pos, n), c);
    }

    /**
     *  @brief  Replace range of characters with range.
     *  @param first  Iterator referencing start of range to replace.
     *  @param last  Iterator referencing end of range to replace.
     *  @param first2  Iterator referencing start of range to insert.
     *  @param last2  Iterator referencing end of range to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [first,last).  In place,
     *  characters in the range [first2,last2) are inserted.  If the
     *  length of result exceeds max_size(), length_error is thrown.
     *  The value of the string doesn't change if an error is
     *  thrown.
     */
    template<class InputIterator>
    basic_inplace_string& replace(iterator first, iterator last, InputIterator first2, InputIterator last2) {
      // TODO : optimization if InputIterator is a RandomIterator
      const basic_inplace_string s(first2, last2);
      const size_type n = last - first;
      __check_size(n, s.size(), "basic_string::replace");

      return __replace_unchecked(first - begin(), n, s.data(), s.size());
    }

    basic_inplace_string& replace(iterator first, iterator last, iterator first2, iterator last2) {
      return replace(first, last, first2, last2);
    }

    basic_inplace_string& replace(iterator first, iterator last, const_iterator first2, const_iterator last2) {
      return replace(first - begin(), first2 - first, last, last2 - last);
    }

    /**
     *  @brief  Replace range of characters with std::initializer_list.
     *  @param first  Iterator referencing start of range to replace.
     *  @param last  Iterator referencing end of range to replace.
     *  @param l  The std::initializer_list of characters to insert.
     *  @return  Reference to this string.
     *  @throw  std::length_error  If new length exceeds @c max_size().
     *
     *  Removes the characters in the range [first,last).  In place,
     *  characters in the range [first2,last2) are inserted.  If the
     *  length of result exceeds max_size(), length_error is thrown.
     *  The value of the string doesn't change if an error is
     *  thrown.
     */
    basic_inplace_string& replace(iterator first, iterator last, std::initializer_list<T> l)
    { return replace(first, last, l.begin(), l.end());}

    /**
     *  @brief  Find position of a C substring.
     *  @param s  C string to locate.
     *  @param pos  Index of character to search from.
     *  @param n  Number of characters from @a s to search for.
     *  @return  Index of start of first occurrence.
     *
     *  Starting from @a pos, searches forward for the first @a
     *  n characters in @a s within this string.  If found,
     *  returns the index where it begins.  If not found, returns
     *  npos.
    */
    size_type find(const T* s, size_type pos, size_type n) const;

    /**
     *  @brief  Find position of a string.
     *  @param str  String to locate.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of start of first occurrence.
     *
     *  Starting from @a pos, searches forward for value of @a str within
     *  this string.  If found, returns the index where it begins.  If not
     *  found, returns npos.
    */
    template<typename Alloc>
    size_type find(const std::basic_string<T, Traits, Alloc>& str, size_type pos = 0) const
    { return find(str.data(), pos, str.size()); }

    /**
     *  @brief  Find position of a C string.
     *  @param s  C string to locate.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of start of first occurrence.
     *
     *  Starting from @a pos, searches forward for the value of @a
     *  s within this string.  If found, returns the index where
     *  it begins.  If not found, returns npos.
    */
    size_type find(const T* s, size_type pos = 0) const {
      return find(s, pos, traits_type::length(s));
    }

    /**
     *  @brief  Find position of a character.
     *  @param c  Character to locate.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for @a c within
     *  this string.  If found, returns the index where it was
     *  found.  If not found, returns npos.
    */
    size_type find(T c, size_type pos = 0) const;

    /**
     *  @brief  Find last position of a string.
     *  @param str  String to locate.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of start of last occurrence.
     *
     *  Starting from @a pos, searches backward for value of @a
     *  str within this string.  If found, returns the index where
     *  it begins.  If not found, returns npos.
    */
    template<typename Alloc>
    size_type rfind(const std::basic_string<T, Traits, Alloc>& str, size_type pos = npos) const
    { return rfind(str.data(), pos, str.size()); }

    /**
     *  @brief  Find last position of a C substring.
     *  @param s  C string to locate.
     *  @param pos  Index of character to search back from.
     *  @param n  Number of characters from s to search for.
     *  @return  Index of start of last occurrence.
     *
     *  Starting from @a pos, searches backward for the first @a
     *  n characters in @a s within this string.  If found,
     *  returns the index where it begins.  If not found, returns
     *  npos.
    */
    size_type rfind(const T* s, size_type pos, size_type n) const;

    /**
     *  @brief  Find last position of a C string.
     *  @param s  C string to locate.
     *  @param pos  Index of character to start search at (default end).
     *  @return  Index of start of  last occurrence.
     *
     *  Starting from @a pos, searches backward for the value of
     *  @a s within this string.  If found, returns the index
     *  where it begins.  If not found, returns npos.
    */
    size_type rfind(const T* s, size_type pos = npos) const {
      return rfind(s, pos, traits_type::length(s));
    }

    /**
     *  @brief  Find last position of a character.
     *  @param c  Character to locate.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for @a c within
     *  this string.  If found, returns the index where it was
     *  found.  If not found, returns npos.
    */
    size_type rfind(T c, size_type pos = npos) const;

    /**
     *  @brief  Find position of a character of string.
     *  @param str  String containing characters to locate.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for one of the
     *  characters of @a str within this string.  If found,
     *  returns the index where it was found.  If not found, returns
     *  npos.
    */
    template<typename Alloc>
    size_type find_first_of(const std::basic_string<T, Traits, Alloc>& str, size_type pos = 0) const
    { return find_first_of(str.data(), pos, str.size()); }

    /**
     *  @brief  Find position of a character of C substring.
     *  @param s  String containing characters to locate.
     *  @param pos  Index of character to search from.
     *  @param n  Number of characters from s to search for.
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for one of the
     *  first @a n characters of @a s within this string.  If
     *  found, returns the index where it was found.  If not found,
     *  returns npos.
    */
    size_type find_first_of(const T* s, size_type pos, size_type n) const;

    /**
     *  @brief  Find position of a character of C string.
     *  @param s  String containing characters to locate.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for one of the
     *  characters of @a s within this string.  If found, returns
     *  the index where it was found.  If not found, returns npos.
    */
    size_type find_first_of(const T* s, size_type pos = 0) const
    { return find_first_of(s, pos, traits_type::length(s)); }

    /**
     *  @brief  Find position of a character.
     *  @param c  Character to locate.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for the character
     *  @a c within this string.  If found, returns the index
     *  where it was found.  If not found, returns npos.
     *
     *  Note: equivalent to find(c, pos).
    */
    size_type find_first_of(T c, size_type pos = 0) const
    { return find(c, pos); }

    /**
     *  @brief  Find last position of a character of string.
     *  @param str  String containing characters to locate.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for one of the
     *  characters of @a str within this string.  If found,
     *  returns the index where it was found.  If not found, returns
     *  npos.
    */
    template<typename Alloc>
    size_type find_last_of(const std::basic_string<T, Traits, Alloc>& str, size_type pos = npos) const
    { return find_last_of(str.data(), pos, str.size()); }

    /**
     *  @brief  Find last position of a character of C substring.
     *  @param s  C string containing characters to locate.
     *  @param pos  Index of character to search back from.
     *  @param n  Number of characters from s to search for.
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for one of the
     *  first @a n characters of @a s within this string.  If
     *  found, returns the index where it was found.  If not found,
     *  returns npos.
    */
    size_type find_last_of(const T* s, size_type pos, size_type n) const;

    /**
     *  @brief  Find last position of a character of C string.
     *  @param s  C string containing characters to locate.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for one of the
     *  characters of @a s within this string.  If found, returns
     *  the index where it was found.  If not found, returns npos.
    */
    size_type find_last_of(const T* s, size_type pos = npos) const
    { return find_last_of(s, pos, traits_type::length(s)); }

    /**
     *  @brief  Find last position of a character.
     *  @param c  Character to locate.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for @a c within
     *  this string.  If found, returns the index where it was
     *  found.  If not found, returns npos.
     *
     *  Note: equivalent to rfind(c, pos).
    */
    size_type find_last_of(T c, size_type pos = npos) const
    { return rfind(c, pos); }

    /**
     *  @brief  Find position of a character not in string.
     *  @param str  String containing characters to avoid.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for a character not contained
     *  in @a str within this string.  If found, returns the index where it
     *  was found.  If not found, returns npos.
    */
    template<typename Alloc>
    size_type find_first_not_of(const std::basic_string<T, Traits, Alloc>& str, size_type pos = 0) const
    { return find_first_not_of(str.data(), pos, str.size()); }

    /**
     *  @brief  Find position of a character not in C substring.
     *  @param s  C string containing characters to avoid.
     *  @param pos  Index of character to search from.
     *  @param n  Number of characters from s to consider.
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for a character not
     *  contained in the first @a n characters of @a s within
     *  this string.  If found, returns the index where it was
     *  found.  If not found, returns npos.
    */
    size_type find_first_not_of(const T* s, size_type pos, size_type n) const;

    /**
     *  @brief  Find position of a character not in C string.
     *  @param s  C string containing characters to avoid.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for a character not
     *  contained in @a s within this string.  If found, returns
     *  the index where it was found.  If not found, returns npos.
    */
    size_type find_first_not_of(const T* s, size_type pos = 0) const
    { return find_first_not_of(s, pos, traits_type::length(s)); }

    /**
     *  @brief  Find position of a different character.
     *  @param c  Character to avoid.
     *  @param pos  Index of character to search from (default 0).
     *  @return  Index of first occurrence.
     *
     *  Starting from @a pos, searches forward for a character
     *  other than @a c within this string.  If found, returns the
     *  index where it was found.  If not found, returns npos.
    */
    size_type find_first_not_of(T c, size_type pos = 0) const;

    /**
     *  @brief  Find last position of a character not in string.
     *  @param str  String containing characters to avoid.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for a character
     *  not contained in @a str within this string.  If found,
     *  returns the index where it was found.  If not found, returns
     *  npos.
    */
    template<size_t N2>
    size_type find_last_not_of(const basic_inplace_string<T, N2, Traits>& str, size_type pos = npos) const
    { return find_last_not_of(str.data(), pos, str.size()); }

    template<typename Alloc>
    size_type find_last_not_of(const std::basic_string<T, Traits, Alloc>& str, size_type pos = npos) const
    { return find_last_not_of(str.data(), pos, str.size()); }

    /**
     *  @brief  Find last position of a character not in C substring.
     *  @param s  C string containing characters to avoid.
     *  @param pos  Index of character to search back from.
     *  @param n  Number of characters from s to consider.
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for a character not
     *  contained in the first @a n characters of @a s within this string.
     *  If found, returns the index where it was found.  If not found,
     *  returns npos.
    */
    size_type find_last_not_of(const T* s, size_type pos, size_type n) const;

    /**
     *  @brief  Find last position of a character not in C string.
     *  @param s  C string containing characters to avoid.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for a character
     *  not contained in @a s within this string.  If found,
     *  returns the index where it was found.  If not found, returns
     *  npos.
    */
    size_type find_last_not_of(const T* s, size_type pos = npos) const
    { return find_last_not_of(s, pos, traits_type::length(s)); }

    /**
     *  @brief  Find last position of a different character.
     *  @param c  Character to avoid.
     *  @param pos  Index of character to search back from (default end).
     *  @return  Index of last occurrence.
     *
     *  Starting from @a pos, searches backward for a character other than
     *  @a c within this string.  If found, returns the index where it was
     *  found.  If not found, returns npos.
    */
    size_type find_last_not_of(T c, size_type pos = npos) const;

    /**
     *  @brief  Get a substring.
     *  @param pos  Index of first character (default 0).
     *  @param n  Number of characters in substring (default remainder).
     *  @return  The new string.
     *  @throw  std::out_of_range  If pos > size().
     *
     *  Construct and return a new string using the @a n
     *  characters starting at @a pos.  If the string is too
     *  short, use the remainder of the characters.  If @a pos is
     *  beyond the end of the string, out_of_range is thrown.
    */
    basic_inplace_string substr(size_type pos = 0, size_type n = npos) const
    { return basic_inplace_string(*this, __check(pos, "basic_inplace_string::substr"), n); }

    /**
     *  @brief  Compare to a string.
     *  @param str  String to compare against.
     *  @return  Integer < 0, 0, or > 0.
     *
     *  Returns an integer < 0 if this string is ordered before @a
     *  str, 0 if their values are equivalent, or > 0 if this
     *  string is ordered after @a str.  Determines the effective
     *  length rlen of the strings to compare as the smallest of
     *  size() and str.size().  The function then compares the two
     *  strings by calling traits::compare(data(), str.data(),rlen).
     *  If the result of the comparison is nonzero returns it,
     *  otherwise the shorter one is ordered first.
    */
    template<typename N2>
    int compare(const basic_inplace_string<T, N2, Traits>& str) const {
      const size_type size = size();
      const size_type size2 = str.size();
      const size_type len = std::min(size, size2);

      int r = traits_type::compare(data(), str.data(), len);
      if (!r) r = __compare(size, size2);

      return r;
    }

    /**
     *  @brief  Compare substring to a string.
     *  @param pos  Index of first character of substring.
     *  @param n  Number of characters in substring.
     *  @param str  String to compare against.
     *  @return  Integer < 0, 0, or > 0.
     *
     *  Form the substring of this string from the @a n characters
     *  starting at @a pos.  Returns an integer < 0 if the
     *  substring is ordered before @a str, 0 if their values are
     *  equivalent, or > 0 if the substring is ordered after @a
     *  str.  Determines the effective length rlen of the strings
     *  to compare as the smallest of the length of the substring
     *  and @a str.size().  The function then compares the two
     *  strings by calling
     *  traits::compare(substring.data(),str.data(),rlen).  If the
     *  result of the comparison is nonzero returns it, otherwise
     *  the shorter one is ordered first.
    */
    template<typename N2>
    int compare(size_type pos, size_type n, const basic_inplace_string<T, N2, Traits>& str) const {
      __check(pos, "basic_inplace_string::compare");
      return __compare_unchecked(data() + pos, __pos_limit(pos, n), str.data(), str.size());
    }

    /**
     *  @brief  Compare substring to a substring.
     *  @param pos1  Index of first character of substring.
     *  @param n1  Number of characters in substring.
     *  @param str  String to compare against.
     *  @param pos2  Index of first character of substring of str.
     *  @param n2  Number of characters in substring of str.
     *  @return  Integer < 0, 0, or > 0.
     *
     *  Form the substring of this string from the @a n1
     *  characters starting at @a pos1.  Form the substring of @a
     *  str from the @a n2 characters starting at @a pos2.
     *  Returns an integer < 0 if this substring is ordered before
     *  the substring of @a str, 0 if their values are equivalent,
     *  or > 0 if this substring is ordered after the substring of
     *  @a str.  Determines the effective length rlen of the
     *  strings to compare as the smallest of the lengths of the
     *  substrings.  The function then compares the two strings by
     *  calling
     *  traits::compare(substring.data(),str.substr(pos2,n2).data(),rlen).
     *  If the result of the comparison is nonzero returns it,
     *  otherwise the shorter one is ordered first.
    */
    template<typename N2>
    int  compare(size_type pos, size_type n,
        const basic_inplace_string<T, N2, Traits>& str, size_type pos2, size_type n2) const {
      __check(pos, "basic_inplace_string::compare");
      str.__check(pos2, "basic_inplace_string::compare");

      return __compare_unchecked(data() + pos, __pos_limit(pos, n), str.data() + pos2, str.__pos_limit(pos2, n2));
    }


    /**
     *  @brief  Compare to a C string.
     *  @param s  C string to compare against.
     *  @return  Integer < 0, 0, or > 0.
     *
     *  Returns an integer < 0 if this string is ordered before @a s, 0 if
     *  their values are equivalent, or > 0 if this string is ordered after
     *  @a s.  Determines the effective length rlen of the strings to
     *  compare as the smallest of size() and the length of a string
     *  constructed from @a s.  The function then compares the two strings
     *  by calling traits::compare(data(),s,rlen).  If the result of the
     *  comparison is nonzero returns it, otherwise the shorter one is
     *  ordered first.
    */
    int compare(const T* s) const
    { return __compare_unchecked(data(), size(), s, traits_type::length(s)); }

    // 5 String::compare specification questionable
    /**
     *  @brief  Compare substring to a C string.
     *  @param pos  Index of first character of substring.
     *  @param n1  Number of characters in substring.
     *  @param s  C string to compare against.
     *  @return  Integer < 0, 0, or > 0.
     *
     *  Form the substring of this string from the @a n1
     *  characters starting at @a pos.  Returns an integer < 0 if
     *  the substring is ordered before @a s, 0 if their values
     *  are equivalent, or > 0 if the substring is ordered after @a
     *  s.  Determines the effective length rlen of the strings to
     *  compare as the smallest of the length of the substring and
     *  the length of a string constructed from @a s.  The
     *  function then compares the two string by calling
     *  traits::compare(substring.data(),s,rlen).  If the result of
     *  the comparison is nonzero returns it, otherwise the shorter
     *  one is ordered first.
    */
    int compare(size_type pos, size_type n, const T* s) const {
      __check(pos, "basic_inplace_string::compare");
      return __compare_unchecked(data() + pos, __pos_limit(pos, n), s, traits_type::length(s));
    }

    /**
     *  @brief  Compare substring against a character %array.
     *  @param pos  Index of first character of substring.
     *  @param n1  Number of characters in substring.
     *  @param s  character %array to compare against.
     *  @param n2  Number of characters of s.
     *  @return  Integer < 0, 0, or > 0.
     *
     *  Form the substring of this string from the @a n1
     *  characters starting at @a pos.  Form a string from the
     *  first @a n2 characters of @a s.  Returns an integer < 0
     *  if this substring is ordered before the string from @a s,
     *  0 if their values are equivalent, or > 0 if this substring
     *  is ordered after the string from @a s.  Determines the
     *  effective length rlen of the strings to compare as the
     *  smallest of the length of the substring and @a n2.  The
     *  function then compares the two strings by calling
     *  traits::compare(substring.data(),s,rlen).  If the result of
     *  the comparison is nonzero returns it, otherwise the shorter
     *  one is ordered first.
     *
     *  NB: s must have at least n2 characters, &apos;\\0&apos; has
     *  no special meaning.
    */
    int compare(size_type pos, size_type n, const T* s, size_type n2) const {
      __check(pos, "basic_inplace_string::compare");
      return __compare_unchecked(data() + pos, __pos_limit(pos, n), s, n2);
    }

  protected:

    size_type __check(size_type pos, const char* s) const {
      if (pos > size()) throw std::out_of_range(s);
      return pos;
    }

    void __check_size(size_type n1, size_type n2, const char* s) const {
      if (max_size() - (size() - n1) < n2)
      throw std::length_error(s);
    }

    bool __disjunct(const T* s) const {
      return (std::less<const T*>()(s, data()) || std::less<const T*>()(data() + size(), s));
    }

    /*
     * Move range [pos + len, size) to range [pos + len2, size - len + len2)
     * note len2 can be less than len
     *
     * Case 1 len2 > len
     *   0              pos       pos+len   pos+len2        size          capacity
     *   |               |           |         |             |               |
     * |-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
     *                               |<--inc-->|
     *                               |.......................|old range
     *                                         |~~~~~~~~~~~~~~~~~~~~~~~|new range   ---->
     *
     * Case 2 len2 <= len
     *
     *   0              pos       pos+len   pos+len2        size          capacity
     *   |               |           |         |             |               |
     * |-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|
     *                     |<--dec-->|
     *                               |.......................|old range
     *   <-----            |~~~~~~~~~~~~~~~~~~~~~~~|new range
     * */
    void __mutate_unckecked(size_type pos, size_type len, size_type len2);

    /*
     * Removes the characters in the range [pos, pos + n). In place,
     * @a n2 characters in src are inserted. Src must not overlap _data
     * */
    basic_inplace_string& __replace_unchecked(size_type pos, size_type n, const T* src, size_type n2);

    /*
     * Removes the characters in the range [pos, pos + n). In place,
     * @a n2 copies of @a c are inserted.
     **/
    basic_inplace_string& __replace_unchecked(size_type pos, size_type n, size_type n2, T c);

    static int __compare_unchecked(const T* s, size_type n, const T* s2, size_type n2) const;

    // When n = 1 way faster than the general multichar
    // traits_type::copy/move/assign.
    static void __fast_copy(T* dest, const T* src, size_type n) {
      if (n == 1) traits_type::assign(*dest, *src);
      else traits_type::copy(dest, src, n);
    }

    static void __fast_copy_backward(T* dest, const T* src, size_type n) {
      if (n == 1) traits_type::assign(*dest, *src);
      else std::copy_backward(src, src + n, dest);
    }

    static void __fast_move(T* dest, const T* src, size_type n) {
      if (n == 1) traits_type::assign(*dest, *src);
      else traits_type::move(dest, src, n);
    }

    static void __fast_assign(T* dest, size_type n, T c) {
      if (n == 1) traits_type::assign(*dest, c);
      else traits_type::assign(dest, n, c);
    }

    static int __compare(size_type n, size_type n2) {
      const difference_type d = difference_type(n - n2);

      if (d > std::numeric_limits<int>::max) return std::numeric_limits<int>::max;
      else if (d < std::numeric_limits<int>::min) return std::numeric_limits<int>::min;
      else return int(d);
    }

    // pos + off can not greater than current string size
    size_type __pos_limit(size_type pos, size_type off) const {
      const bool test_off = off < size() - pos;
      return test_off ? off : size() - pos;
    }

    // pos + off can not greater than current string capacity
    static size_type __size_limit(size_type pos, size_type off) const {
      const bool test_off = off < capacity() - pos;
      return test_off ? off : capacity() - pos;
    }

    void __reset() {
      _size = 0;
      __terminate();
    }

    void __set_size(size_type l) { _size = l; }

    size_type __free_size() const { return capacity() - size(); }

    void __terminate() { _data[_size] = T(); }

    void __greedy_clone(const T* src, size_type pos, size_type n);

  public:

    static const size_type npos = static_cast<size_type>(-1);

  private:

    // Support for zero-length basic_inplace_strings mandatory.
    size_type _size = 0;
    value_type _data[N + 1] = {T()};
  };

  template<typename T, std::size_t N, typename Traits>
  const typename basic_inplace_string<T, N, Traits>::size_type basic_inplace_string<T, N, Traits>::npos;

  // basic_inplace_string comparisons.
  template<typename T, std::size_t N, std::size_t N2, typename Traits>
  inline bool operator==(const basic_inplace_string<T, Traits, N>& one, const basic_inplace_string<T, Traits, N2>& two) {
    return std::equal(one.begin(), one.end(), two.begin());
  }

  template<typename T, std::size_t N, std::size_t N2, typename Traits>
  inline bool operator!=(const basic_inplace_string<T, Traits, N>& one, const basic_inplace_string<T, Traits, N2>& two) {
    return !(one == two);
  }

  template<typename T, std::size_t N, std::size_t N2, typename Traits>
  inline bool operator<(const basic_inplace_string<T, Traits, N>& one, const basic_inplace_string<T, Traits, N2>& two) {
    return std::lexicographical_compare(one.begin(), one.end(), two.begin(), two.end());
  }

  template<typename T, std::size_t N, std::size_t N2, typename Traits>
  inline bool operator>(const basic_inplace_string<T, Traits, N>& one, const basic_inplace_string<T, Traits, N2>& two) {
    return two < one;
  }

  template<typename T, std::size_t N, std::size_t N2, typename Traits>
  inline bool operator<=(const basic_inplace_string<T, Traits, N>& one, const basic_inplace_string<T, Traits, N2>& two) {
    return !(one > two);
  }

  template<typename T, std::size_t N, std::size_t N2, typename Traits>
  inline bool operator>=(const basic_inplace_string<T, Traits, N>& one, const basic_inplace_string<T, Traits, N2>& two) {
    return !(one < two);
  }

  // Specialized algorithms.
  template<typename T, std::size_t N, typename Traits>
  inline void swap(basic_inplace_string<T, Traits, N>& one, basic_inplace_string<T, Traits, N>& two) {
    one.swap(two);
  }

  /**
   *  @brief  Read stream into a string.
   *  @param is  Input stream.
   *  @param str  Buffer to store into.
   *  @return  Reference to the input stream.
   *
   *  Stores characters from @a is into @a str until whitespace is
   *  found, the end of the stream is encountered, or str.max_size()
   *  is reached.  If is.width() is non-zero, that is the limit on the
   *  number of characters stored into @a __str.  Any previous
   *  contents of @a __str are erased.
   */
  template<typename T, std::size_t N, typename Traits>
  inline std::basic_istream<T, Traits>&
  operator>>(std::basic_istream<T, Traits>& is, basic_inplace_string<T, Traits, N>& str) {
    // TODO : optimization
    std::basic_string<T, Traits> tmp;
    is >> tmp;
    str.assign(tmp);
    return is;
  }

  /**
   *  @brief  Write string to a stream.
   *  @param os  Output stream.
   *  @param str  String to write out.
   *  @return  Reference to the output stream.
   *
   *  Output characters of @a str into os following the same rules as for
   *  writing a C string.
   */
  template<typename T, std::size_t N, typename Traits>
  inline std::basic_ostream<T, Traits>&
  operator<<(std::basic_ostream<T, Traits>& os, const basic_inplace_string<T, Traits, N>& str) {
    os.write(str.data(), str.size());
    return os;
  }

} // atlas

#endif /* ATLAS_BASIC_INPLACE_STRING_H_ */

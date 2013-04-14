/*
 * inplace_string_fwd.h
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#ifndef INPLACE_STRING_FWD_H_
#define INPLACE_STRING_FWD_H_

#include <string>

namespace atlas {

  template<typename Ch, size_t N, typename Traits = std::char_traits<Ch>>
  class basic_inplace_string;

//  template<size_t N>
//  typedef basic_inplace_string<char, N> inplace_string;   /// A inplace string of @c char

  typedef basic_inplace_string<char, 4> string4;
  typedef basic_inplace_string<char, 8> string8;
  typedef basic_inplace_string<char, 16> string16;
  typedef basic_inplace_string<char, 32> string32;
  typedef basic_inplace_string<char, 64> string64;
  typedef basic_inplace_string<char, 128> string128;
//  typedef basic_inplace_string<char, 512> string512;
//  typedef basic_inplace_string<char, 1024> string1024;
//  typedef basic_inplace_string<char, 2048> string2048;
//  typedef basic_inplace_string<char, 4096> string4096;
//  typedef basic_inplace_string<char, 8192> string8192;
//  typedef basic_inplace_string<char, 16384> string16384;

//  template<size_t N>
//  typedef basic_inplace_string<wchar_t, N> inplace_wstring;   /// A inplace string of @c wchar_t
//
//  typedef basic_inplace_string<wchar_t, 4> wstring4;
//  typedef basic_inplace_string<wchar_t, 8> wstring8;
//  typedef basic_inplace_string<wchar_t, 16> wstring16;
//  typedef basic_inplace_string<wchar_t, 32> wstring32;
//  typedef basic_inplace_string<wchar_t, 64> wstring64;
//  typedef basic_inplace_string<wchar_t, 128> wstring128;
//  typedef basic_inplace_string<wchar_t, 512> wstring512;
//  typedef basic_inplace_string<wchar_t, 1024> wstring1024;
//  typedef basic_inplace_string<wchar_t, 2048> wstring2048;
//  typedef basic_inplace_string<wchar_t, 4096> wstring4096;
//  typedef basic_inplace_string<wchar_t, 8192> wstring8192;
//  typedef basic_inplace_string<wchar_t, 16384> wstring16384;
//
////  template<size_t N>
////  typedef basic_inplace_string<char16_t, N> inplace_u16string; /// A inplace string of @c char16_t
//
//  typedef basic_inplace_string<char16_t, 4> u16string4;
//  typedef basic_inplace_string<char16_t, 8> u16string8;
//  typedef basic_inplace_string<char16_t, 16> u16string16;
//  typedef basic_inplace_string<char16_t, 32> u16string32;
//  typedef basic_inplace_string<char16_t, 64> u16string64;
//  typedef basic_inplace_string<char16_t, 128> u16string128;
//  typedef basic_inplace_string<char16_t, 512> u16string512;
//  typedef basic_inplace_string<char16_t, 1024> u16string1024;
//  typedef basic_inplace_string<char16_t, 2048> u16string2048;
//  typedef basic_inplace_string<char16_t, 4096> u16string4096;
//  typedef basic_inplace_string<char16_t, 8192> u16string8192;
//  typedef basic_inplace_string<char16_t, 16384> u16string16384;
//
////  template<size_t N>
////  typedef basic_inplace_string<char32_t, N> inplace_u32string; /// A inplace string of @c char32_t
//
//  typedef basic_inplace_string<char32_t, 4> u32string4;
//  typedef basic_inplace_string<char32_t, 8> u32string8;
//  typedef basic_inplace_string<char32_t, 16> u32string16;
//  typedef basic_inplace_string<char32_t, 32> u32string32;
//  typedef basic_inplace_string<char32_t, 64> u32string64;
//  typedef basic_inplace_string<char32_t, 128> u32string128;
//  typedef basic_inplace_string<char32_t, 512> u32string512;
//  typedef basic_inplace_string<char32_t, 1024> u32string1024;
//  typedef basic_inplace_string<char32_t, 2048> u32string2048;
//  typedef basic_inplace_string<char32_t, 4096> u32string4096;
//  typedef basic_inplace_string<char32_t, 8192> u32string8192;
//  typedef basic_inplace_string<char32_t, 16384> u32string16384;

// TODO : use template alias
//  template<size_t N>
//  using inplace_u32string = basic_inplace_string<char32_t, N>;

#define ATLAS_TYPEDEF_INPLACE_STRING(N)   typedef basic_inplace_string<char, N> string##N;
#define ATLAS_TYPEDEF_INPLACE_WSTRING(N)   typedef basic_inplace_string<wchar_t, N> wstring##N;
#define ATLAS_TYPEDEF_INPLACE_U16STRING(N)   typedef basic_inplace_string<char16_t, N> u16string##N;
#define ATLAS_TYPEDEF_INPLACE_U32STRING(N)   typedef basic_inplace_string<char32_t, N> u32string##N;

}// atlas

#endif /* INPLACE_STRING_FWD_H_ */

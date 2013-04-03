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

  template<size_t N>
  typedef basic_inplace_string<char, N> inplace_string;   /// A inplace string of @c char

  typedef inplace_string<4> string4;
  typedef inplace_string<8> string8;
  typedef inplace_string<16> string16;
  typedef inplace_string<32> string32;
  typedef inplace_string<64> string64;
  typedef inplace_string<128> string128;
  typedef inplace_string<512> string512;
  typedef inplace_string<1024> string1024;
  typedef inplace_string<2048> string2048;
  typedef inplace_string<4096> string4096;
  typedef inplace_string<8192> string8192;
  typedef inplace_string<16384> string16384;

  template<size_t N>
  typedef basic_inplace_string<wchar_t, N> inplace_wstring;   /// A inplace string of @c wchar_t

  typedef inplace_wstring<4> wstring4;
  typedef inplace_wstring<8> wstring8;
  typedef inplace_wstring<16> wstring16;
  typedef inplace_wstring<32> wstring32;
  typedef inplace_wstring<64> wstring64;
  typedef inplace_wstring<128> wstring128;
  typedef inplace_wstring<512> wstring512;
  typedef inplace_wstring<1024> wstring1024;
  typedef inplace_wstring<2048> wstring2048;
  typedef inplace_wstring<4096> wstring4096;
  typedef inplace_wstring<8192> wstring8192;
  typedef inplace_wstring<16384> wstring16384;

  template<size_t N>
  typedef basic_inplace_string<char16_t, N> inplace_u16string; /// A inplace string of @c char16_t

  typedef inplace_u16string<4> u16string4;
  typedef inplace_u16string<8> u16string8;
  typedef inplace_u16string<16> u16string16;
  typedef inplace_u16string<32> u16string32;
  typedef inplace_u16string<64> u16string64;
  typedef inplace_u16string<128> u16string128;
  typedef inplace_u16string<512> u16string512;
  typedef inplace_u16string<1024> u16string1024;
  typedef inplace_u16string<2048> u16string2048;
  typedef inplace_u16string<4096> u16string4096;
  typedef inplace_u16string<8192> u16string8192;
  typedef inplace_u16string<16384> u16string16384;

  template<size_t N>
  typedef basic_inplace_string<char32_t, N> inplace_u32string; /// A inplace string of @c char32_t

  typedef inplace_u16string<4> u32string4;
  typedef inplace_u32string<8> u32string8;
  typedef inplace_u32string<16> u32string16;
  typedef inplace_u32string<32> u32string32;
  typedef inplace_u32string<64> u32string64;
  typedef inplace_u32string<128> u32string128;
  typedef inplace_u32string<512> u32string512;
  typedef inplace_u32string<1024> u32string1024;
  typedef inplace_u32string<2048> u32string2048;
  typedef inplace_u32string<4096> u32string4096;
  typedef inplace_u32string<8192> u32string8192;
  typedef inplace_u32string<16384> u32string16384;

// TODO : use template alias
//  template<size_t N>
//  using inplace_u32string = basic_inplace_string<char32_t, N>;

}// atlas

#endif /* INPLACE_STRING_FWD_H_ */

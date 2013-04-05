/*
 * inplace_string.cpp
 *
 *  Created on: Mar 31, 2013
 *      Author: vincent
 */

#define BOOST_TEST_MODULE inplace_string

#include <boost/test/unit_test.hpp>
#include <atlas/inplace_string.h>

using atlas::string4;
using atlas::string8;
using atlas::string16;
using atlas::string32;
using atlas::string64;
using atlas::string128;

const char* words = "This is the end";
std::string words2 = "Hold your breath and count to ten";
std::string words3 = "Feel the earth move and then";
std::string words4 = "Hear my heart burst again";
std::string words5 = "For this is the end";
std::string words6 = "I've drowned and dreamed this moment";
std::string words7 = "So overdue, I owe them";
std::string words8 = "Swept away, I'm stolen";
std::string words9 = "Let the sky fall, when it crumbles";

BOOST_AUTO_TEST_SUITE(inplace_string)

BOOST_AUTO_TEST_CASE(construction)
{
  // empty string
  string64 s;
  BOOST_CHECK(s.empty());

  // construct from cstring
  string64 s2(words);
  BOOST_CHECK(s2 == words);

  // construct from string
  string64 s3(words2);
  BOOST_CHECK(s3 == words2);

  // construct from inplace_string with the same capacity
  string64 s4(s2);
  BOOST_CHECK(s2 == words);

  // construct from inplace_string with bigger capacity
  string128 s5(s3);
  BOOST_CHECK(s5 == words2);

  // construct from inplace_string with smaller capacity
  string4 s6(s3);
  BOOST_CHECK(s6 != words2);
  BOOST_CHECK(s6 == words2.substr(0, 4));

  string64 s7(words, 0, 4);
  BOOST_CHECK(s7 == "This");

  string64 s8(words2, 1, 4);
  BOOST_CHECK(s8 == "his ");

  string64 s9(s2, 0, 4);
  BOOST_CHECK(s9 == "This");
}

BOOST_AUTO_TEST_CASE(construction_assignment)
{
  // assignment cotr from cstring
  string64 s = words;
  BOOST_CHECK(s == words);

  // assignment cotr from string
  string64 s2 = words2;
  BOOST_CHECK(s2 == words2);

  // construct from inplace_string with the same capacity
  string64 s3 = s2;
  BOOST_CHECK(s3 == words2);

  // construct from inplace_string with the bigger capacity
  string128 s4 = s2;
  BOOST_CHECK(s4 == words2);

  // construct from inplace_string with the smaller capacity
  string4 s5 = s2;
  BOOST_CHECK(s5 != words2);
  BOOST_CHECK(s5 == words2.substr(0, 4));
}

BOOST_AUTO_TEST_CASE(construction_truncate)
{
  string4 s(words, 0);
  string4 s2(words, 1);
  string4 s3(words, 5);

  BOOST_CHECK(s == "");
  BOOST_CHECK(s2 == "T");
  BOOST_CHECK(s3 == "This");
}

BOOST_AUTO_TEST_CASE(construction_fill)
{
  string8 s(4, 'a');
  string128 s2(100, 'a');

  BOOST_CHECK(s == "aaaa");
  BOOST_CHECK(s2 == "aaaa");
}

BOOST_AUTO_TEST_CASE(construction_range)
{
  string16 s("0123456789123456");
  string32 s2(s.begin(), s.end());
  string8 s3(s.begin(), s.end());

  BOOST_CHECK(s2 == s);
  BOOST_CHECK(s3 == "01234567");
}

BOOST_AUTO_TEST_CASE(resize)
{
  string16 s("0123456789123456");

  s.resize(8);
  BOOST_CHECK(s == "01234567");

  s.resize(10, 'a');
  BOOST_CHECK(s == "01234567aa");
}

BOOST_AUTO_TEST_CASE(append)
{
  string16 s("0123456789");
  string4 s2("1234");

  // append a string
  s.append(s2);
  BOOST_CHECK(s == "01234567891234");

  // append a string, with truncation occurs
  s.append(s2);
  BOOST_CHECK(s == "0123456789123412");

  // append a string, truncate to zero size
  s.append(s2);
  BOOST_CHECK(s == "0123456789123412");
}

BOOST_AUTO_TEST_CASE(append_string)
{
  string16 s("0123456789");
  std::string s2("1234");

  // append a string
  s.append(s2);
  BOOST_CHECK(s == "01234567891234");

  // append a string, with truncation occurs
  s.append(s2);
  BOOST_CHECK(s == "0123456789123412");

  // append a string, truncate to zero size
  s.append(s2);
  BOOST_CHECK(s == "0123456789123412");
}

BOOST_AUTO_TEST_CASE(insert)
{
  string16 s("056789");
  const char* s2 = "1234";

  // insert a string
  s.insert(1, s2);
  BOOST_CHECK(s == "0123456789");

  // insert a string, with truncation occurs
  s.insert(s.size(), s2);
  BOOST_CHECK(s == "0123456789123412");

  // append a string, truncate to zero size
  s.insert(s.size(), s2);
  BOOST_CHECK(s == "0123456789123412");
}

BOOST_AUTO_TEST_CASE(insert_string)
{
  string16 s("056789");
  std::string s2 = "1234";

  // insert a string
  s.insert(1, s2);
  BOOST_CHECK(s == "0123456789");

  // insert a string, with truncation occurs
  s.insert(s.size(), s2);
  BOOST_CHECK(s == "0123456789123412");

  // append a string, truncate to zero size
  s.insert(s.size(), s2);
  BOOST_CHECK(s == "0123456789123412");
}

BOOST_AUTO_TEST_CASE(erase)
{
  string16 s("0123456789");
  auto first = s.begin() + 1;
  auto last = s.begin() + 4;

  s.erase(first, last);
  BOOST_CHECK(s == "056789");
}

BOOST_AUTO_TEST_CASE(replace_cstring)
{
  string16 s("0123456789123456");
  const char* s2 = "abcdefghijk";
  const char* s3 = "abcd";

  s.replace(1, 4, s2, 5);
  BOOST_CHECK(s == "0abcde56789123456");

  s.replace(1, 6, s3, 10000);
  BOOST_CHECK(s == "0abcd789123456");
}

BOOST_AUTO_TEST_CASE(replace_string)
{
  string16 s("0123456789123456");
  std::string s2 = "abcdefghijk";
  std::string s3 = "abcd";

  s.replace(1, 4, s2, 5);
  BOOST_CHECK(s == "0abcde56789123456");

  s.replace(1, 6, s3, 10000);
  BOOST_CHECK(s == "0abcd789123456");
}

BOOST_AUTO_TEST_SUITE_END()

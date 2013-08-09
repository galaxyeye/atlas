//          Copyright Stefan Strasser 2010  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_CONFIG_HPP
#define BOOST_TRANSACT_DETAIL_CONFIG_HPP

#include <boost/config.hpp>

//work around a bug in MSVC that doesn't allow the template keyword
//to be used in call to template operators.
//see boost list thread:
//"[boost] [config] explicit call of operator templates / msvc bugworkaround"
#ifdef BOOST_MSVC
#define BOOST_NESTED_OPERATOR_TEMPLATE
#else
#define BOOST_NESTED_OPERATOR_TEMPLATE template
#endif

#endif

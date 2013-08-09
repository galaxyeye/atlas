//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_TRANSACT_LANGUAGE_HPP
#define BOOST_TRANSACT_LANGUAGE_HPP

#include <boost/transact/transaction.hpp>

#define begin_transaction BOOST_TRANSACT_BEGIN_TRANSACTION
#define retry BOOST_TRANSACT_RETRY
#define end_retry BOOST_TRANSACT_END_RETRY
#define end_retry_in_loop BOOST_TRANSACT_END_RETRY_IN_LOOP
#define end_transaction BOOST_TRANSACT_END_TRANSACTION
#define end_transaction_in_loop BOOST_TRANSACT_END_TRANSACTION_IN_LOOP

#endif

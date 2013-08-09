//          Copyright Stefan Strasser 2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_TRANSACT_TRANSACTION_HEADER_HPP
#define BOOST_TRANSACT_TRANSACTION_HEADER_HPP

#include <boost/transact/basic_transaction.hpp>
#include <boost/transact/transaction_manager.hpp>


namespace boost{
namespace transact{

/// \brief An alias of \c basic_transaction using the default transaction manager
typedef basic_transaction<transaction_manager> transaction;


}
}

#define BOOST_TRANSACT_BEGIN_TRANSACTION \
    BOOST_TRANSACT_BASIC_BEGIN_TRANSACTION(boost::transact::transaction_manager)

#define BOOST_TRANSACT_RETRY \
    BOOST_TRANSACT_BASIC_RETRY(boost::transact::transaction_manager)

#define BOOST_TRANSACT_END_RETRY \
    BOOST_TRANSACT_BASIC_END_RETRY(boost::transact::transaction_manager)

#define BOOST_TRANSACT_END_RETRY_IN_LOOP \
    BOOST_TRANSACT_BASIC_END_RETRY_IN_LOOP(boost::transact::transaction_manager)

#define BOOST_TRANSACT_END_TRANSACTION \
    BOOST_TRANSACT_BASIC_END_TRANSACTION(boost::transact::transaction_manager)

#define BOOST_TRANSACT_END_TRANSACTION_IN_LOOP \
    BOOST_TRANSACT_BASIC_END_TRANSACTION_IN_LOOP(boost::transact::transaction_manager)

#endif

//          Copyright Stefan Strasser 2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_TRANSACT_TRANSACTION_MANAGER_HPP
#define BOOST_TRANSACT_TRANSACTION_MANAGER_HPP


#ifndef BOOST_TRANSACT_CONFIGURATION
#error BOOST_TRANSACT_CONFIGURATION not defined
#endif

namespace boost{
namespace transact{

/// \brief An alias of the configured transaction manager
typedef BOOST_TRANSACT_CONFIGURATION transaction_manager;


}
}

#endif

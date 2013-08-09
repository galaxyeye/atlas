//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_MUTEX_HPP
#define BOOST_TRANSACT_DETAIL_MUTEX_HPP

namespace boost{
namespace transact{
namespace detail{

struct null_lockable{
    void lock(){}
    bool try_lock(){ return true; }
    void unlock(){}
};

}
}
}

#endif


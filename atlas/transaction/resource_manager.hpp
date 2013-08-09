//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_RESOURCE_MANAGER_HPP
#define BOOST_TRANSACT_RESOURCE_MANAGER_HPP

#include <boost/transact/default_tag.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/sequence_tag.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/has_key.hpp>

namespace boost {
  namespace transact {

    struct restart_transaction_service_tag {};
    struct finish_transaction_service_tag {};
    struct nested_transaction_service_tag {};
    struct distributed_transaction_service_tag {};

    namespace detail {

      template<class Tag>
      struct has_service_impl {
        template<class Resource, class Service>
        struct apply {
          typedef typename mpl::contains<typename Resource::services, Service>::type type;
        };
      };

      template<>
      struct has_service_impl<mpl::sequence_tag<mpl::set0<> >::type> {
        template<class Resource, class Service>
        struct apply {
          typedef typename mpl::has_key<typename Resource::services, Service>::type type;
        };
      };
    }

    template<class Resource, class Service>
    struct has_service :
    detail::has_service_impl<typename mpl::sequence_tag<typename Resource::services>::type>::template apply<
        Resource, Service> {
    };

  } // transact
} // boost

#endif

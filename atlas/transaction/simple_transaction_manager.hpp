//          Copyright Stefan Strasser 2010.
//      Copyright Vicente J. Botet Escriba 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_SIMPLE_TRANSACTION_MANAGER_HEADER_HPP
#define BOOST_TRANSACT_SIMPLE_TRANSACTION_MANAGER_HEADER_HPP

#include <boost/mpl/map.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/ref.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/optional/optional.hpp>
#include <boost/transact/exception.hpp>
#include <boost/noncopyable.hpp>
#include <boost/transact/detail/static_tss.hpp>
#include <boost/transact/detail/transaction_manager.hpp>
#include <boost/transact/resource_manager.hpp>

namespace boost {
  namespace transact {

    template<class Resource, bool FlatNested = false, bool Threads = true, class ResourceTag = default_tag>
    class simple_transaction_manager;

    namespace detail {

      namespace simple_transaction_manager {

        template<class Resource>
        class resource_transactions : noncopyable {
        public:

          resource_transactions() :
              rolled_back_(false) {
          }
          bool rolled_back() const {
            return this->rolled_back_;
          }
          void rolled_back(bool rb) {
            this->rolled_back_ = rb;
          }
          optional<typename Resource::transaction> &get() {
            return this->tx;
          }

        private:

          optional<typename Resource::transaction> tx;
          bool rolled_back_;
        };

        template<class Tag, class Resource>
        class transaction : noncopyable {
        public:

          template<class T>
          transaction(T const &c) : parent_(c.parent) {
            resources &ress = c.resources;
            this->construct(ress, typename T::flat());
          }

        private:

          typedef detail::tag_resource_pair<Tag, Resource> resources;

//public:
          template<class, bool, bool, class > friend class transact::simple_transaction_manager;

          typename Resource::transaction &resource_transaction(resources &ress, Tag const &tag) {
            Resource &res = ress.get(tag);
            optional<typename Resource::transaction> &rtx = this->rtxs->get();
            BOOST_ASSERT(rtx);
            return *rtx;
          }

          void commit(resources &ress) {
            if (this->flat_nested()) {
              if (this->rtxs->rolled_back()) throw no_transaction();
              return; // flat nested transaction commit is a no-op
            }
            else {
              if (!ress.empty()) {
                BOOST_ASSERT(this->rtxs_storage && this->rtxs == &*this->rtxs_storage);
                finisher finish;
                BOOST_VERIFY(!finish(ress.tag(), ress.get(ress.tag()), this->rtxs_storage->get(), false));
                committer commit;
                commit(ress.tag(), ress.get(ress.tag()), this->rtxs_storage->get());
              }
            }
          }

          void rollback(resources &ress) {
            if (!this->rtxs->rolled_back()) { //a flat nested transaction might have rolled back before
              this->rtxs->rolled_back(true);
              rollbacker rollback;
              rollback(ress.tag(), ress.get(ress.tag()), this->rtxs->get());
            }
          }

          void restart(resources &ress) {
            if (this->flat_nested()) throw unsupported_operation();
            BOOST_ASSERT(this->rtxs_storage);
            beginner<transaction, resources> begin(this->parent(), ress);
            restarter<beginner<transaction, resources> > restart(begin);
            restart(ress.tag(), ress.get(ress.tag()), this->rtxs_storage->get());
          }

          transaction *parent() const { return this->parent_; }

        private:

          void construct(resources &ress, mpl::true_ flat) {
            if (this->parent()) {
              this->rtxs = this->parent()->rtxs;
              BOOST_ASSERT(this->rtxs);
              if (this->rtxs->rolled_back()) {
                //user tried to begin a transaction in a closed transaction scope that was not restarted even though
                //an open transaction inside it was rolled back:
                throw no_transaction();
              }
            }
            else this->construct_closed(ress);
          }

          void construct(resources &ress, mpl::false_ flat) {
            this->construct_closed(ress);
          }

          friend struct transact::detail::beginner<transaction, resources>;

          void construct_closed(resources &ress) {
            this->rtxs_storage = in_place();
            this->rtxs = &*this->rtxs_storage;
            beginner<transaction, resources> begin(this->parent(), ress);
            begin(ress.tag(), ress.get(ress.tag()), this->rtxs_storage->get());
          }

          bool flat_nested() const {
            return this->parent() && this->parent()->rtxs == this->rtxs;
          }

          resource_transactions<Resource> *rtxs; // never 0. == *rtxs_storage for closed transactions, == *parent->parent->...->rtxs_storage for open transactions.
          transaction *parent_;
          optional<resource_transactions<Resource> > rtxs_storage;
        };

      }

    }

/// Model of TransactionManager. Only members that are not part of that concept are documented here.
///
/// Equivalent to <tt>basic_transaction_manager<mpl::vector<Resource>,Threads></tt>.
///
/// Template parameters:
/// \li \c Resource The type of the resource manager used.
/// \li \c FlatNested Use emulated flat nested transactions instead of the NestedTransactionService offered by the ResourceManager.
/// \li \c Threads \c true if multiple threads are used to access this transaction manager.
/// \li \c ResourceTag The tag that identifies the Resource
// \brief A transaction manager that only supports one resource manager.
    template<class Resource, bool FlatNested, bool Threads, class ResourceTag>
    class simple_transaction_manager : noncopyable {
      /// \cond
    private:
      struct detail { //for QuickBook
        typedef transact::detail::simple_transaction_manager::transaction<ResourceTag, Resource> transaction;
        typedef transact::detail::tag_resource_pair<ResourceTag, Resource> resources_type;
        class transaction_construct_t {
          transaction_construct_t(resources_type &resources, transaction *parent) :
              resources(resources), parent(parent) {
          }
          friend class simple_transaction_manager;
          template<class, class > friend class transact::detail::simple_transaction_manager::transaction;
          resources_type &resources;
          transaction * const parent;
          typedef mpl::bool_<FlatNested> flat; //TODO
        };
      };
      struct currenttx_tag {
      };
      typedef transact::detail::static_thread_specific_ptr<typename detail::transaction, currenttx_tag, Threads> currenttx;
      /// \endcond
    public:
      typedef typename detail::transaction transaction;
      typedef mpl::map1<mpl::pair<ResourceTag, Resource> > resource_types;

      /// \brief Constructs a simple_transaction_manager
      simple_transaction_manager() { }

      /// TODO doc, not part of the concept
      static void connect_resource(Resource &newres, ResourceTag const &tag = ResourceTag()) {
        resources_.connect(tag, newres);
      }

      /// TODO doc, not part of the concept
      /// \pre No transactions must be active
      static void disconnect_resource(ResourceTag const &tag = ResourceTag()) { resources_.disconnect(tag); }

      /// \pre A resource manager must be connected
      static Resource &resource(ResourceTag const &tag = ResourceTag()) { return resources_.get(tag); }

      template<class Tag>
      struct resource_iterator {
        typedef typename detail::resources_type::iterator type;
      };

      template<class Tag>
      static std::pair<typename resource_iterator<Tag>::type, typename resource_iterator<Tag>::type> resources() {
        return resources_.range();
      }

      static typename Resource::transaction&
      resource_transaction(transaction &tx, ResourceTag const &tag = ResourceTag()) {
        return tx.resource_transaction(resources_, tag);
      }

      /// \pre A resource manager must be connected
      static typename detail::transaction_construct_t begin_transaction() {
        return typename detail::transaction_construct_t(resources_, currenttx::get());
      }

      static void commit_transaction(transaction &tx) {
        bind_transaction(tx);
        tx.commit(resources_);
      }

      static void rollback_transaction(transaction &tx) { tx.rollback(resources_); }

      static void restart_transaction(transaction &tx) { tx.restart(resources_); }

      static void bind_transaction(transaction &tx) { currenttx::reset(&tx); }

      static void unbind_transaction() { currenttx::reset(0); }

      static transaction *current_transaction() { return currenttx::get(); }

      static transaction *parent_transaction(transaction &tx) { return tx.parent(); }

      /// \cond
    private:
      typedef typename detail::resources_type resources_type;
      static resources_type resources_;
      /// \endcond
    };

    template<class Resource, bool FlatNested, bool Threads, class ResourceTag>
    typename simple_transaction_manager<Resource, FlatNested, Threads, ResourceTag>::resources_type simple_transaction_manager<
        Resource, FlatNested, Threads, ResourceTag>::resources_;

  }
}

#endif

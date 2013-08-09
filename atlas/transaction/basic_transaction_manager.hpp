//          Copyright Stefan Strasser 2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_BASIC_TRANSACTION_MANAGER_HEADER_HPP
#define BOOST_TRANSACT_BASIC_TRANSACTION_MANAGER_HEADER_HPP

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/static_assert.hpp>
#include <boost/optional/optional.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/empty_sequence.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/container/map/convert.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/unordered_map.hpp>
#include <boost/scoped_array.hpp>
#include <functional>

#include <boost/transact/detail/mutex.hpp>
#include <boost/transact/exception.hpp>
#include <boost/transact/detail/static_tss.hpp>
#include <boost/transact/detail/algorithm.hpp>
#include <boost/transact/detail/transaction_manager.hpp>
#include <boost/transact/resource_manager.hpp>

namespace boost {
  namespace transact {

    template<class Resources, bool FlatNested = false, class Lazy = mpl::empty_sequence, bool Threads = true,
        bool TThreads = true> class basic_transaction_manager;

    namespace detail {

      namespace basic_transaction_manager {

        template<class Tag, class Resource>
        class tag_resource_map : noncopyable {
        private:
          typedef unordered_map<Tag, std::pair<Resource *, std::size_t> > map_type;
          struct transformer : std::unary_function<typename map_type::value_type const, std::pair<Tag, Resource *> > {
            std::pair<Tag, Resource *> operator()(typename map_type::value_type const &in) const {
              return std::pair<Tag, Resource *>(in.first, in.second.first);
            }
          };
        public:
          typedef transform_iterator<transformer, typename map_type::const_iterator> iterator;
          void connect(Tag const &tag, Resource &res) {
            if (!this->map.insert(std::make_pair(tag, std::make_pair(&res, 0))).second) throw resource_error();
            this->reindex();
          }
          void disconnect(Tag const &tag) {
            if (this->map.erase(tag) != 1) throw resource_error();
            this->reindex();
          }
          Resource &get(Tag const &tag) const {
            typename map_type::const_iterator it = this->map.find(tag);
            if (it == this->map.end()) throw resource_error();
            BOOST_ASSERT(it->second.first);
            return *it->second.first;
          }
          Resource &get(Tag const &tag, std::size_t &index) const {
            typename map_type::const_iterator it = this->map.find(tag);
            if (it == this->map.end()) throw resource_error();
            BOOST_ASSERT(it->second.first);
            index = it->second.second;
            return *it->second.first;
          }
          std::pair<iterator, iterator> range() const {
            return std::pair<iterator, iterator>(iterator(this->map.begin(), transformer()),
                iterator(this->map.end(), transformer()));
          }
          std::size_t size() const {
            return this->map.size();
          }
          bool empty() const {
            return this->map.empty();
          }
        private:
          void reindex() {
            //unordered containers have ForwardIterators, this implies that there is a defined order as long as the container isn`t modified:
            std::size_t index = 0;
            for (typename map_type::iterator it = this->map.begin(); it != this->map.end(); ++it) {
              it->second.second = index++;
            }
          }

          map_type map;
        };

        template<class Resources>
        class resources : noncopyable {
        private:
          template<class Pair>
          struct make_tag_resource_pair {
            typedef fusion::pair<typename Pair::first, tag_resource_pair<typename Pair::first, typename Pair::second> > type;
          };
          template<class Pair>
          struct make_tag_resource_map {
            typedef fusion::pair<typename Pair::first, tag_resource_map<typename Pair::first, typename Pair::second> > type;
          };
          template<class Pair>
          struct tag_is_empty {
            typedef is_empty<typename Pair::first> type;
          };
          typedef typename fusion::result_of::as_map<
              typename mpl::fold<Resources, mpl::vector0<>,
                  mpl::push_back<mpl::_1,
                      mpl::if_<tag_is_empty<mpl::_2>, make_tag_resource_pair<mpl::_2>, make_tag_resource_map<mpl::_2> > > >::type>::type type_map_type;
          template<class Tag, class Resource>
          struct resource_type {
            typedef typename mpl::if_<is_empty<Tag>, tag_resource_pair<Tag, Resource>, tag_resource_map<Tag, Resource> >::type type;
          };
        public:
          template<class Tag>
          typename mpl::at<Resources, Tag>::type &get(Tag const &tag) {
            return fusion::at_key<Tag>(this->type_map).get(tag);
          }
          template<class Tag>
          typename mpl::at<Resources, Tag>::type &get(Tag const &tag, std::size_t &index) {
            return fusion::at_key<Tag>(this->type_map).get(tag, index);
          }
          template<class Tag>
          void connect(Tag const &tag, typename mpl::at<Resources, Tag>::type &res) {
            fusion::at_key<Tag>(this->type_map).connect(tag, res);
          }
          template<class Tag>
          void disconnect(Tag const &tag) {
            fusion::at_key<Tag>(this->type_map).disconnect(tag);
          }
          template<class Tag>
          struct iterator {
            typedef typename mpl::at<Resources, Tag>::type Resource;
            typedef typename resource_type<Tag, Resource>::type::iterator type;
          };
          template<class Tag>
          std::pair<typename iterator<Tag>::type, typename iterator<Tag>::type> range() const {
            return fusion::at_key<Tag>(this->type_map).range();
          }
          template<class Tag>
          std::size_t size() const {
            return fusion::at_key<Tag>(this->type_map).size();
          }
        private:
          type_map_type type_map;
        };

        template<class Resources>
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
          template<class Tag>
          optional<typename mpl::at<Resources, Tag>::type::transaction> &get(std::size_t index) {
            return fusion::at_key<Tag>(this->tx).get(index);
          }
          template<class Tag>
          void size(std::size_t size) {
            fusion::at_key<Tag>(this->tx).size(size);
          }
        private:
          template<class Resource>
          class transaction {
          public:
            optional<typename Resource::transaction> &get(std::size_t index) {
              BOOST_ASSERT(index == 0);
              return this->tx;
            }
            void size(std::size_t size) {
              BOOST_ASSERT(size <= 1);
            }
          private:
            optional<typename Resource::transaction> tx;
          };
          template<class Resource>
          class transaction_array {
          public:
            optional<typename Resource::transaction> &get(std::size_t index) {
              return this->tx[index];
            }
            void size(std::size_t size) {
              if (size > 0) {
                this->tx.reset(new optional<typename Resource::transaction> [size]);
              }
            }
          private:
            scoped_array<optional<typename Resource::transaction> > tx;
          };
          template<class Pair>
          struct make_transaction {
            typedef fusion::pair<typename Pair::first, transaction<typename Pair::second> > type;
          };
          template<class Pair>
          struct make_transaction_array {
            typedef fusion::pair<typename Pair::first, transaction_array<typename Pair::second> > type;
          };
          template<class Pair>
          struct tag_is_empty {
            typedef is_empty<typename Pair::first> type;
          };
          typedef typename fusion::result_of::as_map<
              typename mpl::fold<Resources, mpl::vector0<>,
                  mpl::push_back<mpl::_1,
                      mpl::if_<tag_is_empty<mpl::_2>, make_transaction<mpl::_2>, make_transaction_array<mpl::_2> > > >::type>::type tx_type;
          tx_type tx;
          bool rolled_back_;
        };

        template<class It, class End, class Resources, class F>
        void for_each(resources<Resources> &, resource_transactions<Resources> &, F &, mpl::true_ end) {
        }

        template<class It, class End, class Resources, class F>
        void for_each(resources<Resources> &ress, resource_transactions<Resources> &txs, F &f, mpl::false_ end) {
          typedef typename mpl::deref<It>::type::first Tag;
          typedef typename mpl::deref<It>::type::second Resource;
          typedef typename resources<Resources>::template iterator<Tag>::type iterator;
          std::pair<iterator, iterator> range = ress.template range<Tag>();
          std::size_t index = 0;
          for (iterator it = range.first; it != range.second; ++it, ++index) {
            optional<typename Resource::transaction> &tx = txs.get<Tag>(index);
            f(it->first, *it->second, tx);
          }

          typedef typename mpl::next<It>::type Next;
          for_each<Next, End>(ress, txs, f, typename is_same<Next, End>::type());
        }

        template<class Resources, class F>
        void for_each(resources<Resources> &ress, resource_transactions<Resources> &txs, F f) {
          for_each<typename mpl::begin<Resources>::type, typename mpl::end<Resources>::type>(ress, txs, f,
              typename mpl::empty<Resources>::type());
        }

        template<class F, class State>
        struct folder {
        public:
          explicit folder(F const &f, State &state) :
              f(f), state(state) {
          }
          template<class Tag, class Resource>
          void operator()(Tag const &tag, Resource const &res, optional<typename Resource::transaction> &tx) {
            this->state = this->f(tag, res, tx, this->state);
          }
        private:
          F f;
          State &state;
        };

        template<class Resources, class State, class F>
        State fold(resources<Resources> &ress, resource_transactions<Resources> &txs, F const &f, State state) {
          for_each<Resources>(ress, txs, folder<F, State>(f, state));
          return state;
        }

        template<class Resources, class LazySet, bool TThreads>
        class transaction : noncopyable {
        public:
          template<class T>
          transaction(T const &c) :
              parent_(c.parent) {
            resources<Resources> &ress = c.resources;
            this->construct(ress, typename T::flat());
          }
        private:
//public:
          template<class, bool, class, bool, bool> friend class transact::basic_transaction_manager;

          template<class Tag>
          typename mpl::at<Resources, Tag>::type::transaction &resource_transaction(resources<Resources> &ress,
              Tag const &tag) {
            typedef typename mpl::at<Resources, Tag>::type Resource;
            std::size_t index;
            Resource &res = ress.get(tag, index);
            optional<typename Resource::transaction> &rtx = this->rtxs->get<Tag>(index);
            this->lazy_begin(tag, res, rtx, ress, typename mpl::has_key<LazySet, Tag>::type());
            BOOST_ASSERT(rtx);
            return *rtx;
          }
          void commit(resources<Resources> &ress) {
            if (this->flat_nested()) {
              if (this->rtxs->rolled_back()) throw no_transaction();
              return; // flat nested transaction commit is a no-op
            }
            else {
              BOOST_ASSERT(this->rtxs_storage && this->rtxs == &*this->rtxs_storage);
              //finish until all finish_transaction()s return false
              while (basic_transaction_manager::fold(ress, *this->rtxs_storage, finisher(), false))
                ;

              for_each(ress, *this->rtxs_storage, preparer<false>()); //prepare non-durable two-phase transactions
              //count durable transactions and transactions that only support one-phase commit:
              std::size_t dur = basic_transaction_manager::fold(ress, *this->rtxs_storage, durable_counter(),
                  std::size_t(0));
              if (dur > 1) {
                for_each(ress, *this->rtxs_storage, preparer<true>()); //prepare durable transactions
                //TODO write commit message to log
              }
              //commit durable transactions first, because there might have been only
              //one durable transaction, which might still fail because it was not prepared:
              for_each(ress, *this->rtxs_storage, committer<true>());
              for_each(ress, *this->rtxs_storage, committer<false>());
            }
          }

          void rollback(resources<Resources> &ress) {
            if (!this->rtxs->rolled_back()) { //a flat nested transaction might have rolled back before
              this->rtxs->rolled_back(true);
              for_each(ress, *this->rtxs, rollbacker());
            }
          }
          void restart(resources<Resources> &ress) {
            if (this->flat_nested()) throw unsupported_operation();
            BOOST_ASSERT(this->rtxs_storage);
            beginner<false> begin(this->parent(), ress);
            for_each(ress, *this->rtxs_storage, restarter<beginner<false> >(begin));
          }

          transaction *parent() const {
            return this->parent_;
          }
        private:
          void construct(resources<Resources> &ress, mpl::true_ flat) {
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
          void construct(resources<Resources> &ress, mpl::false_ flat) {
            this->construct_closed(ress);
          }
          void construct_closed(resources<Resources> &ress) {
            this->rtxs_storage = in_place();
            this->rtxs = &*this->rtxs_storage;
            detail::for_each<Resources>(rtx_sizer(*this->rtxs_storage, ress));
            for_each(ress, *this->rtxs_storage, beginner<false>(this->parent(), ress));
          }

          struct rtx_sizer {
            rtx_sizer(resource_transactions<Resources> &txs, resources<Resources> &ress) :
                txs(txs), ress(ress) {
            }
            template<class Pair>
            void operator()() {
              //make room for as many resource transactions as there are resources connected:
              typedef typename Pair::first Tag;
              txs.size<Tag>(ress.size<Tag>());
            }
          private:
            resource_transactions<Resources> &txs;
            resources<Resources> &ress;
          };
          friend struct transact::detail::beginner<transaction, resources<Resources> >;
          template<bool ConstructLazy>
          struct beginner : transact::detail::beginner<transaction, resources<Resources> > {
            typedef transact::detail::beginner<transaction, resources<Resources> > base_type;

            beginner(transaction *parent, resources<Resources> &ress) :
                base_type(parent, ress) {
            }
            template<class Tag, class Resource>
            void operator()(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx) {
              static bool const lazy = mpl::has_key<LazySet, Tag>::type::value && !ConstructLazy;
              this->begin(tag, res, tx, mpl::bool_<lazy>());
            }
          private:
            template<class Tag, class Resource>
            void begin(Tag const &, Resource &, optional<typename Resource::transaction> &tx, mpl::true_ lazy) {
              BOOST_ASSERT(!tx);
            }
            template<class Tag, class Resource>
            void begin(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx, mpl::false_ lazy) {
              BOOST_ASSERT(!tx);
              base_type::operator()(tag, res, tx);
              BOOST_ASSERT(tx);
            }
          };
          template<bool Durable>
          struct preparer {
            template<class Tag, class Resource>
            void operator()(Tag const &, Resource &res, optional<typename Resource::transaction> &tx) {
              if (tx)
                this->prepare(res, *tx, typename has_service<Resource, distributed_transaction_service_tag>::type());
            }
          private:
            template<class Resource>
            void prepare(Resource &res, typename Resource::transaction &rtx, mpl::true_ distributedservice) {
              if (Durable == Resource::durable) {
                res.prepare_transaction(rtx);
              }
            }
            template<class Resource>
            void prepare(Resource &, typename Resource::transaction &, mpl::false_ distributedservice) {
              if (Durable) throw unsupported_operation();
            }
          };
          template<bool Durable>
          struct committer : transact::detail::committer {
            template<class Tag, class Resource>
            void operator()(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx) {
              this->commit(tag, res, tx, typename has_service<Resource, distributed_transaction_service_tag>::type());
            }
          private:
            template<class Tag, class Resource>
            void commit(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx,
                mpl::true_ distributedservice) {
              if (Durable == Resource::durable) {
                transact::detail::committer::operator()(tag, res, tx);
              }
            }
            template<class Tag, class Resource>
            void commit(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx,
                mpl::false_ distributedservice) {
              if (Durable) transact::detail::committer::operator()(tag, res, tx);
            }
          };
          struct durable_counter {
            template<class Tag, class Resource>
            std::size_t operator()(Tag const &, Resource &, optional<typename Resource::transaction> &tx,
                std::size_t count) {
              return count
                  + (tx
                      && this->durable<Resource>(
                          typename has_service<Resource, distributed_transaction_service_tag>::type()) ? 1 : 0);
            }
          private:
            template<class Resource>
            bool durable(mpl::true_ distributedservice) const {
              return Resource::durable;
            }
            template<class Resource>
            bool durable(mpl::false_ distributedservice) const {
              return true;
            }
          };
          bool flat_nested() const {
            return this->parent() && this->parent()->rtxs == this->rtxs;
          }

          template<class Tag, class Resource>
          void lazy_begin(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx,
              resources<Resources> &ress, mpl::true_ lazy) {
            lock_guard<mutex_type> l(this->mutex);
            if (!tx) {
              beginner<true> begin(this->parent, ress);
              begin(tag, res, tx);
            }
            BOOST_ASSERT(tx);
          }
          template<class Tag, class Resource>
          void lazy_begin(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx,
              resources<Resources> &ress, mpl::false_ lazy) {
          }

          resource_transactions<Resources> *rtxs; // never 0. == *rtxs_storage for closed transactions, == *parent->parent->...->rtxs_storage for open transactions.
          transaction *parent_;
          optional<resource_transactions<Resources> > rtxs_storage;

          typedef typename mpl::if_c<TThreads && !mpl::empty<LazySet>::value, boost::mutex,
              transact::detail::null_lockable>::type mutex_type;
          mutex_type mutex;
        };

      }

    }

/// Model of TransactionManager. Only members that are not part of that concept are documented here.
///
/// Template parameters:
/// \li \c Resources A MPL Sequence containing the types of the resource managers used.
/// \li \c Lazy A MPL Sequence of resource tags of those resource managers whose transactions
///        ought to be started lazily, i.e. the local transaction of the resource manager
///        is not started when the global transaction is started but on first
///        access of the resource transaction. This can be beneficial when 2 or more
///        resource managers are used but not every resource is accessed in a global
///        transaction.
/// \li \c Threads \c true if multiple threads are used to access this transaction manager.
/// \li \c TThreads \c true if multiple threads are used to access the same transaction. Can be \c false if multiple threads are used to access the transaction manager, but not to access the same transaction.
/// \brief A transaction manager
    template<class Resources, bool FlatNested, class Lazy, bool Threads, bool TThreads>
    class basic_transaction_manager : noncopyable {
/// \cond
      BOOST_STATIC_ASSERT(Threads || !TThreads);
    private:
      struct detail { //for QuickBook
        typedef typename mpl::fold< //TODO optimization: dont fold if Resources already is a mpl::map
            Resources, mpl::map0<>, mpl::insert<mpl::_1, mpl::_2> >::type resource_types;
        typedef typename mpl::fold< //TODO optimization: dont fold if Lazy already is a mpl::set
            Lazy, mpl::set0<>, mpl::insert<mpl::_1, mpl::_2> >::type lazy_set;
        typedef transact::detail::basic_transaction_manager::transaction<resource_types, lazy_set, TThreads> transaction;
        typedef typename transact::detail::basic_transaction_manager::resources<resource_types> resources_type;
        class transaction_construct_t {
          transaction_construct_t(resources_type &resources, transaction *parent) :
              resources(resources), parent(parent) {
          }
          friend class basic_transaction_manager;
          template<class, class, bool> friend class transact::detail::basic_transaction_manager::transaction;
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
      typedef typename detail::resource_types resource_types;

      /// A basic_transaction_manager constructed using this constructor
      /// is not able to commit transactions that involve two or more persistent
      /// resource managers, i.e. that require a two phase commit protocol.
      /// \brief Constructs a basic_transaction_manager
      basic_transaction_manager() {
      }

      /// TODO doc, not part of the concept
      template<class Tag>
      static void connect_resource(typename mpl::at<resource_types, Tag>::type &newres, Tag const &tag = Tag()) {
        resources_.connect(tag, newres);
      }

      /// TODO doc, not part of the concept
      template<class ResourceTag>
      static void disconnect_resource(ResourceTag tag = ResourceTag()) {
        resources_.disconnect(tag);
      }

      template<class Tag>
      static typename mpl::at<resource_types, Tag>::type &resource(Tag tag = Tag()) {
        return resources_.get(tag);
      }

      template<class Tag>
      struct resource_iterator {
        typedef typename detail::resources_type::template iterator<Tag>::type type;
      };
      template<class Tag>
      static std::pair<typename resource_iterator<Tag>::type, typename resource_iterator<Tag>::type> resources() {
        return resources_.template range<Tag>();
      }

      template<class Tag>
      static typename mpl::at<resource_types, Tag>::type::transaction &
      resource_transaction(transaction &tx, Tag tag = Tag()) {
        return tx.resource_transaction(resources_, tag);
      }

      static typename detail::transaction_construct_t begin_transaction() {
        return typename detail::transaction_construct_t(resources_, currenttx::get());
      }

      static void commit_transaction(transaction &tx) {
        bind_transaction(tx);
        tx.commit(resources_);
      }

      static void rollback_transaction(transaction &tx) {
        tx.rollback(resources_);
      }
      static void restart_transaction(transaction &tx) {
        tx.restart(resources_);
      }

      static void bind_transaction(transaction &tx) {
        currenttx::reset(&tx);
      }
      static void unbind_transaction() {
        currenttx::reset(0);
      }

      static transaction *current_transaction() {
        return currenttx::get();
      }
      static transaction *parent_transaction(transaction &tx) {
        return tx.parent();
      }

      /// \cond
    private:
      typedef typename detail::resources_type resources_type;
      static resources_type resources_;
      /// \endcond
    };

    template<class Resources, bool FlatNested, class Lazy, bool Threads, bool TThreads>
    typename basic_transaction_manager<Resources, FlatNested, Lazy, Threads, TThreads>::resources_type basic_transaction_manager<
        Resources, FlatNested, Lazy, Threads, TThreads>::resources_;

  }
}

#endif

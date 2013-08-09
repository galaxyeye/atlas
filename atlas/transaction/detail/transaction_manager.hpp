//          Copyright Stefan Strasser 2010.
//      Copyright Vicente J. Botet Escriba 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_TAG_RESOURCE_PAIR_HPP
#define BOOST_TRANSACT_DETAIL_TAG_RESOURCE_PAIR_HPP

#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <boost/optional/optional.hpp>
#include <boost/transact/exception.hpp>
#include <boost/noncopyable.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/transact/resource_manager.hpp>
#include <boost/transact/exception.hpp>

namespace boost {
  namespace transact {

    namespace detail {

      template<class Tag, class Resource>
      class tag_resource_pair : noncopyable {
      public:

        typedef std::pair<Tag, Resource *> const *iterator;

        void connect(Tag const &tag, Resource &res) {
          if (this->pair) {
            if (this->tag_is_equal(tag)) throw resource_error();
            else throw unsupported_operation();
          }
          this->pair = std::make_pair(tag, &res);
        }

        void disconnect(Tag const &tag) {
          if (!this->pair || !this->tag_is_equal(tag)) throw resource_error();
          this->pair = none;
        }

        Resource &get(Tag const &tag) const {
          if (!this->pair || !this->tag_is_equal(tag)) throw resource_error();
          BOOST_ASSERT(this->pair->second);
          return *this->pair->second;
        }

        Resource &get(Tag const &tag, std::size_t &index) const {
          if (!this->pair || !this->tag_is_equal(tag)) throw resource_error();
          BOOST_ASSERT(this->pair->second);
          index = 0;
          return *this->pair->second;
        }

        std::pair<iterator, iterator> range() const {
          if (this->pair) {
            return std::pair<iterator, iterator>(&*this->pair, &*this->pair + 1);
          }
          else {
            return std::pair<iterator, iterator>(0, 0);
          }
        }

        bool empty() const { return !this->pair; }

        std::size_t size() const { return this->pair ? 1 : 0; }

        const Tag& tag() const {
          BOOST_ASSERT(this->pair);
          return this->pair->first;
        }

      private:

        bool tag_is_equal(Tag const &o) const {
          return this->tag_is_equal(o, boost::is_empty<Tag>());
        }

        bool tag_is_equal(Tag const &, mpl::true_ empty) const {
          return true;
        }

        bool tag_is_equal(Tag const &o, mpl::false_ empty) const {
          BOOST_ASSERT(this->pair);
          return this->pair->first == o;
        }

        optional<std::pair<Tag, Resource *> > pair;
      };

      template<class Transaction, class Resources>
      struct beginner {

        beginner(Transaction *parent, Resources &ress) :
            parent(parent), ress(ress) {
        }

        template<class Tag, class Resource>
        void operator()(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx) {
          BOOST_ASSERT(!tx);
          if (this->parent) this->begin_nested(tag, res, tx,
              typename has_service<Resource, nested_transaction_service_tag>::type());
          else tx = in_place(res.begin_transaction());
          BOOST_ASSERT(tx);
        }

      private:

        template<class Tag, class Resource>
        void begin_nested(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx,
            mpl::true_ nestedservice) {
          BOOST_ASSERT(this->parent);
          typename Resource::transaction &parenttx = this->parent->resource_transaction(this->ress, tag);
          tx = in_place(res.begin_nested_transaction(parenttx));
        }

        template<class Tag, class Resource>
        void begin_nested(Tag const &, Resource &, optional<typename Resource::transaction> &,
            mpl::false_ nestedservice) {
          throw unsupported_operation();
        }

        Transaction *parent;
        Resources &ress;
      };

      struct finisher {
        template<class Tag, class Resource>
        bool operator()(Tag const &, Resource &res, optional<typename Resource::transaction> &tx, bool repeat) {
          if (tx) {
            return this->finish(res, *tx, typename has_service<Resource, finish_transaction_service_tag>::type())
                || repeat;
          }
          else return repeat;
        }

      private:

        template<class Resource>
        bool finish(Resource &res, typename Resource::transaction &tx, mpl::true_ finishservice) {
          return res.finish_transaction(tx);
        }

        template<class Resource>
        bool finish(Resource &res, typename Resource::transaction &tx, mpl::false_ finishservice) {
          return false;
        }
      };

      struct committer {
        template<class Tag, class Resource>
        void operator()(Tag const &, Resource &res, optional<typename Resource::transaction> &tx) {
          if (tx) res.commit_transaction(*tx);
        }
      };

      struct rollbacker {
        template<class Tag, class Resource>
        void operator()(Tag const &, Resource &res, optional<typename Resource::transaction> &tx) {
          if (tx) res.rollback_transaction(*tx);
        }
      };

      template<class Beginner>
      struct restarter {
        explicit restarter(Beginner const &begin) :
            begin(begin) {
        }

        template<class Tag, class Resource>
        void operator()(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx) {
          if (tx) this->restart(tag, res, tx, typename has_service<Resource, restart_transaction_service_tag>::type());
        }

      private:

        template<class Tag, class Resource>
        void restart(Tag const &, Resource &res, optional<typename Resource::transaction> &tx, mpl::true_ service) {
          BOOST_ASSERT(tx);
          res.restart_transaction(*tx);
        }

        template<class Tag, class Resource>
        void restart(Tag const &tag, Resource &res, optional<typename Resource::transaction> &tx, mpl::false_ service) {
          BOOST_ASSERT(tx);
          res.rollback_transaction(*tx);
          tx = none;
          this->begin(tag, res, tx);
        }

        Beginner begin;
      };

    }
  }
}

#endif

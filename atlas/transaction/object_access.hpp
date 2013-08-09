//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_OBJECT_ACCESS_HPP
#define BOOST_TRANSACT_OBJECT_ACCESS_HPP

#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/wrapper.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/transact/detail/embedded_vector.hpp>
#include <cstring>
#include <algorithm>
#include <iterator>

#ifndef NO_BOOST_SERIALIZATION

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/archive/impl/archive_serializer_map.ipp>

#endif

namespace boost {
  namespace transact {

    struct deep_tag { };

    namespace detail {

      template<class T>
      struct construct_tag;
      template<class Archive, class T>
      void construct(Archive &, T *&, deep_tag);
      template<class UnaryFunction>
      class apply_archive;
    }

    class object_access {
    public:

      template<class Archive, class T>
      static void save(Archive &ar, T const &t) {
        object_access::save(ar, t, empty<T>(), serialization::is_bitwise_serializable<T>());
      }

      template<class Archive, class T>
      static void construct(Archive &ar, T *&t) {
        object_access::construct(ar, t, empty<T>(), serialization::is_bitwise_serializable<T>());
      }

      template<class Archive, class T>
      static void load(Archive &ar, T &t) {
        object_access::load(ar, t, empty<T>(), serialization::is_bitwise_serializable<T>());
      }

      template<class T>
      static T *clone(T const &t) {
        return object_access::clone(t, empty<T>(), serialization::is_bitwise_serializable<T>());
      }

      template<class T>
      static bool equal(T const &t1, T const &t2) {
        return object_access::equal(t1, t2, empty<T>(), serialization::is_bitwise_serializable<T>());
      }

      template<class UnaryFunction, class T>
      static void apply(UnaryFunction const &f, T const &t) {
        object_access::apply(f, t, empty<T>(), serialization::is_bitwise_serializable<T>());
      }

    private:

      template<class T>
      struct empty : mpl::bool_<boost::is_empty<T>::value && !serialization::is_wrapper<T>::type::value> {
      };

      template<class T>
      friend struct detail::construct_tag;

      template<class Archive, class T>
      friend void detail::construct(Archive &, T *&, deep_tag);

      template<class T>
      static void construct(T &t) {
        new (&t) T();
      }

      template<class T>
      static void destruct(T &t) {
        t.~T();
      }

      template<class UnaryFunction, class T, bool Bitwise>
      static void apply(UnaryFunction const &f, T const &t, mpl::true_ empty, mpl::bool_<Bitwise>) {
        f(t);
      }

      template<class UnaryFunction, class T>
      static void apply(UnaryFunction const &f, T const &t, mpl::false_ empty, mpl::true_ bitwise) {
        f(t);
      }

      template<class UnaryFunction, class T>
      static void apply(UnaryFunction const &f, T const &t, mpl::false_ empty, mpl::false_ bitwise);

      template<class UnaryFunction, class T>
      static void apply(detail::apply_archive<UnaryFunction> const &ar, T const &t, mpl::false_ empty,
          mpl::false_ bitwise);

      template<class Archive, class T, bool Bitwise>
      static void save(Archive &ar, T const &t, mpl::true_ empty, mpl::bool_<Bitwise>) {
      }

      template<class Archive, class T>
      static void save(Archive &ar, T const &t, mpl::false_ empty, mpl::true_ bitwise) {
        ar.save_binary(&t, mpl::size_t<sizeof(T)>());
      }

      template<class Archive, class T>
      static void save(Archive &ar, T const &t, mpl::false_ empty, mpl::false_ bitwise);

      template<class Archive, class T, bool Bitwise>
      static void load(Archive &ar, T &t, mpl::true_ empty, mpl::bool_<Bitwise>) {
      }

      template<class Archive, class T>
      static void load(Archive &ar, T &t, mpl::false_ empty, mpl::true_ bitwise) {
        ar.load_binary(&t, mpl::size_t<sizeof(T)>());
      }

      template<class Archive, class T>
      static void load(Archive &ar, T &t, mpl::false_ empty, mpl::false_ bitwise);

      template<class Archive, class T, bool Bitwise>
      static void construct(Archive &ar, T *&t, mpl::true_ empty, mpl::bool_<Bitwise>) {
        t = static_cast<T *>(::operator new(sizeof(T)));
      }

      template<class Archive, class T>
      static void construct(Archive &ar, T *&t, mpl::false_ empty, mpl::true_ bitwise) {
        t = static_cast<T *>(::operator new(sizeof(T)));
        try {
          ar.load_binary(t, mpl::size_t<sizeof(T)>());
        }
        catch (...) {
          ::operator delete(t);
          throw;
        }
      }
      template<class Archive, class T>
      static void construct(Archive &ar, T *&t, mpl::false_ empty, mpl::false_ bitwise);

      template<class T, bool Bitwise>
      static T *clone(T const &t, mpl::true_ empty, mpl::bool_<Bitwise>) {
        return static_cast<T *>(::operator new(sizeof(T)));
      }

      template<class T>
      static T *clone(T const &t, mpl::false_ empty, mpl::true_ bitwise) {
        T *tmp = static_cast<T *>(::operator new(sizeof(T)));
        std::memcpy(tmp, &t, sizeof(T));
        return tmp;
      }

      template<class T>
      static T *clone(T const &t, mpl::false_ empty, mpl::false_ bitwise);

      template<class T, bool Bitwise>
      static bool equal(T const &, T const &, mpl::true_ empty, mpl::bool_<Bitwise>) {
        return true;
      }

      template<class T>
      static bool equal(T const &t1, T const &t2, mpl::false_ empty, mpl::true_ bitwise) {
        return std::memcmp(&t1, &t2, sizeof(T)) == 0;
      }

      template<class T>
      static bool equal(T const &t1, T const &t2, mpl::false_ empty, mpl::false_ bitwise);
    };

  }
}

#include <boost/transact/archive.hpp>

namespace boost {
  namespace transact {
    namespace detail {

      template<class T>
      struct boost_serialization_required;

      template<class Archive, class T>
      void apply(Archive const &ar, T const &t, deep_tag) {
#ifdef NO_BOOST_SERIALIZATION
        sizeof(boost_serialization_required<T>);
#else
        serialization::serialize_adl(const_cast<Archive &>(ar).serialization_archive(), const_cast<T &>(t),
            serialization::version<T>::value);
#endif
      }

      template<class UnaryFunction, class T>
      void apply_(UnaryFunction const &f, T const &t, deep_tag tag) {
#if __GNUC__ && ((__GNUC__== 4 && __GNUC_MINOR__ <= 4) || __GNUC__ < 4)
        //GCC <= 4.4 looks up ADL names in namespaces that are only used as template arguments,
        //which causes a conflict with boost::mpl::apply.
        //apply is an optimization only, ignore user-supplied apply functions and use serialization instead:
        detail::apply(f,t,tag);
#else
        apply(f, t, tag);
#endif
      }

      template<class UnaryFunction, class T>
      void apply_adl(UnaryFunction const &f, T const &t) {
        apply_(f, t, deep_tag());
      }

      struct constructed_tag {
        constructed_tag(deep_tag) {
        }
      };

      struct unconstructed_tag {
      public:

        explicit unconstructed_tag(bool &constr) :
            constructed_(constr) {
          BOOST_ASSERT(!this->constructed());
        }

        bool &constructed() const {
          return this->constructed_;
        }

      private:
        bool& constructed_;
      };

      template<class T>
      struct construct_tag {
        construct_tag(T &t, bool &constructed) :
            t(t), constructed(constructed) {
        }
        operator deep_tag() const {
          //a user-supplied load-function is called. construct:
          transact::object_access::construct(this->t);
          this->constructed = true;
          return deep_tag();
        }
        operator unconstructed_tag() const {
          return unconstructed_tag(this->constructed);
        }
      private:
        T &t;
        bool &constructed;
      };

      template<class Archive, class T>
      void serialize(Archive &ar, T &t, mpl::true_ saving) {
        archive::save(ar.serialization_archive(), t);
      }
      template<class Archive, class T>
      void serialize(Archive &ar, T &t, mpl::false_ saving) {
        archive::load(ar.serialization_archive(), t);
      }

      template<class Archive, class T>
      void serialize(Archive &ar, T &t, constructed_tag) {
#ifdef NO_BOOST_SERIALIZATION
        sizeof(boost_serialization_required<T>);
#else
        detail::serialize(ar, t, typename Archive::is_saving());
#endif
      }

      template<class Archive, class T>
      void serialize_(Archive &ar, T &t, constructed_tag) {
        serialize(ar, t, deep_tag());
      }

      template<class Archive, class T>
      void serialize(Archive volatile &var, T &t, unconstructed_tag tag) {
#ifdef NO_BOOST_SERIALIZATION
        sizeof(boost_serialization_required<T>);
#else
        Archive &ar = (Archive &) var;
        BOOST_STATIC_ASSERT(Archive::is_loading::value);
        serialization::load_construct_data_adl(ar.serialization_archive(), &t, serialization::version<T>::value);
        tag.constructed() = true;
        serialization::serialize_adl(ar.serialization_archive(), t, serialization::version<T>::value);
#endif
      }

      template<class Archive, class T>
      void serialize_(Archive volatile &ar, T &t, unconstructed_tag tag) {
        serialize((Archive &) ar, t, construct_tag<T>(t, tag.constructed()));
      }

      template<class Archive, class T>
      void serialize_adl(Archive &ar, T &t) {
        serialize_(ar, t, deep_tag());
      }

      template<class Archive, class T>
      void load(Archive &ar, T &t, constructed_tag) {
        BOOST_STATIC_ASSERT(Archive::is_loading::value);
        serialize_adl(ar, t);
      }

      template<class Archive, class T>
      void load_(Archive &ar, T &t, constructed_tag) {
        load(ar, t, deep_tag());
      }

      template<class Archive, class T>
      void load(Archive volatile &ar, T &t, unconstructed_tag tag) {
        serialize_((Archive &) ar, t, construct_tag<T>(t, tag.constructed()));
      }

      template<class Archive, class T>
      void load_(Archive volatile &ar, T &t, unconstructed_tag tag) {
        load((Archive &) ar, t, construct_tag<T>(t, tag.constructed()));
      }

      template<class Archive, class T>
      void load_adl(Archive &ar, T &t) {
        load_(ar, t, deep_tag());
      }

      template<class Archive, class T>
      void construct(Archive &ar, T *&t, deep_tag tag) {
        void *data = ::operator new(sizeof(T));
        T &tmp = *static_cast<T *>(data);
        bool constructed = false;
        try {
          load_(ar, tmp, construct_tag<T>(tmp, constructed));
        }
        catch (...) {
          if (constructed) transact::object_access::destruct(tmp);
          ::operator delete(data);
          throw;
        }
        t = &tmp;
      }

      template<class Archive, class T>
      void construct_(Archive &ar, T *&t, deep_tag tag) {
        construct(ar, t, tag);
      }

      template<class Archive, class T>
      void construct_adl(Archive &ar, T *&t) {
        construct_(ar, t, deep_tag());
      }

      template<class Archive, class T>
      void save(Archive &ar, T const &t, deep_tag) {
        BOOST_STATIC_ASSERT(Archive::is_saving::value);
        detail::serialize_adl(ar, const_cast<T &>(t));
      }

      template<class Archive, class T>
      void save_(Archive &ar, T const &t, deep_tag) {
        save(ar, t, deep_tag());
      }

      template<class Archive, class T>
      void save_adl(Archive &ar, T const &t) {
        save_(ar, t, deep_tag());
      }

      template<class T>
      T *clone(T const &t, deep_tag) {
        typedef detail::embedded_vector<char, 256, true> buffer_type;
        buffer_type buffer;
        {
          typedef vector_back_insert_iterator<buffer_type> iterator;
          iterator it(buffer);
          detail::memory_oarchive<iterator> ar(it);
          object_access::save(ar, t);
        }
        {
          detail::memory_iarchive<buffer_type::iterator> ar(buffer.begin(), buffer.end());
          T *tmp = 0;
          object_access::construct(ar, tmp);
          BOOST_ASSERT(tmp);
          return tmp;
        }
      }

      template<class T>
      T *clone_(T const &t, deep_tag) {
        return clone(t, deep_tag());
      }

      template<class T>
      T *clone_adl(T const &t) {
        return clone_(t, deep_tag());
      }

      template<class T>
      bool equal(T const &t1, T const &t2, deep_tag) {
        typedef detail::embedded_vector<char, 256, true> buffer_type;
        buffer_type buffer;
        {
          typedef vector_back_insert_iterator<buffer_type> iterator;
          iterator it(buffer);
          detail::memory_oarchive<iterator> ar(it);
          object_access::save(ar, t1);
        }
        {
          detail::compare_archive<buffer_type::iterator> ar(buffer.begin(), buffer.end());
          object_access::save(ar, t2);
          return ar.equal();
        }
      }

      template<class T>
      bool equal_(T const &t1, T const &t2, deep_tag) {
        return equal(t1, t2, deep_tag());
      }

      template<class T>
      bool equal_adl(T const &t1, T const &t2) {
        return equal_(t1, t2, deep_tag());
      }

    }

    template<class UnaryFunction, class T>
    void object_access::apply(UnaryFunction const &f, T const &t, mpl::false_ empty, mpl::false_ bitwise) {
      detail::apply_archive<UnaryFunction> ar(f);
      detail::apply_adl(ar, t);
    }
    template<class UnaryFunction, class T>
    void object_access::apply(detail::apply_archive<UnaryFunction> const &ar, T const &t, mpl::false_ empty,
        mpl::false_ bitwise) {
      detail::apply_adl(ar, t);
    }

    template<class Archive, class T>
    void object_access::construct(Archive &ar, T *&t, mpl::false_ empty, mpl::false_ bitwise) {
      detail::construct_adl(ar, t);
    }

    template<class Archive, class T>
    void object_access::load(Archive &ar, T &t, mpl::false_ empty, mpl::false_ bitwise) {
      detail::load_adl(ar, t);
    }

    template<class Archive, class T>
    void object_access::save(Archive &ar, T const &t, mpl::false_ empty, mpl::false_ bitwise) {
      detail::save_adl(ar, t);
    }

    template<class T>
    T *object_access::clone(T const &t, mpl::false_ empty, mpl::false_ bitwise) {
      return detail::clone_adl(t);
    }

    template<class T>
    bool object_access::equal(T const &t1, T const &t2, mpl::false_ empty, mpl::false_ bitwise) {
      return detail::equal_adl(t1, t2);
    }

  }
}

#endif

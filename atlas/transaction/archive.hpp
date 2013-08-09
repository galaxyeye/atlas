//          Copyright Stefan Strasser 2010 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_ARCHIVE_HPP
#define BOOST_TRANSACT_ARCHIVE_HPP

#include <boost/optional/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/ref.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/transact/array_extension.hpp>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <typeinfo>

#ifndef NO_BOOST_SERIALIZATION

#include <boost/archive/basic_binary_iarchive.hpp>
#include <boost/archive/basic_binary_oarchive.hpp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/serialization/wrapper.hpp>
#include <string>

#endif

namespace boost {
  namespace transact {

    namespace detail {

#ifndef NO_BOOST_SERIALIZATION

      template<class Base>
      class serialization_oarchive : public archive::basic_binary_oarchive<serialization_oarchive<Base> > {
        typedef archive::basic_binary_oarchive<serialization_oarchive<Base> > base_type;
      public:
        explicit serialization_oarchive(Base &ar) :
            base_type(archive::no_header | archive::no_codecvt), base(ar) {
        }

        void save_binary(void const *data, std::size_t size) {
          this->base.save_binary(data, size);
        }
        template<class T>
        void save_override(T const &t, int) {
          this->base << t;
        }
        using base_type::save_override; //for serialization primitives -> binary conversion

        template<class T>
        void save(T const &t) {
          this->base << t;  //boost.serialization can save serialization primitives bypassing override, e.g. class names
        }
      private:
        Base &base;
      };

      template<class Base>
      class serialization_iarchive : public archive::basic_binary_iarchive<serialization_iarchive<Base> > {
        typedef archive::basic_binary_iarchive<serialization_iarchive<Base> > base_type;
      public:
        explicit serialization_iarchive(Base &ar) :
            base_type(archive::no_header | archive::no_codecvt), base(ar) {
        }

        void load_binary(void *data, std::size_t size) {
          this->base.load_binary(data, size);
        }
        template<class T>
        void load_override(T &t, int) {
          this->load(t);
        }
        using base_type::load_override;

        template<class T>
        void load(T &t) {
          this->load(t, typename serialization::is_wrapper<T>::type());
        }
      private:
        template<class T>
        void load(T const &t, mpl::true_ wrapper) {
          this->base >> const_cast<T &>(t);
        }
        template<class T>
        void load(T &t, mpl::false_ wrapper) {
          this->base >> t;
        }

        Base &base;
      };

#endif

    }

    template<class Derived>
    class basic_oarchive {
    public:
      typedef mpl::true_ is_saving;
      typedef mpl::false_ is_loading;
      template<class T>
      basic_oarchive &operator&(T const &t) {
        return that().operator<<(t);
      }
      template<class T>
      basic_oarchive &operator<<(T const &t) {
        that().save(t);
        return *this;
      }
      template<class T>
      void save(T const &t);
      void save(std::string const &s) {
        std::size_t const size = s.size();
        that() << size;
        if (size > 0) that().save_binary(s.data(), size);
      }
      void save(std::wstring const &s) {
        std::size_t const size = s.size();
        that() << size;
        if (size > 0) that().save_binary(s.data(), size * sizeof(wchar_t));
      }
      void save(char const *s) {
        std::size_t const size = std::strlen(s);
        that() << size;
        if (size > 0) that().save_binary(s, size);
      }
      void save(wchar_t const *s) {
        std::size_t const size = std::wcslen(s);
        that() << size;
        if (size > 0) that().save_binary(s, size * sizeof(wchar_t));
      }
    private:
      Derived &that() {
        return static_cast<Derived &>(*this);
      }
#ifndef NO_BOOST_SERIALIZATION
    public:
      detail::serialization_oarchive<Derived> &serialization_archive() {
        if (!this->sarchive) {
          this->sarchive = in_place(boost::ref(that()));
        }
        return *this->sarchive;
      }
    private:
      optional<detail::serialization_oarchive<Derived> > sarchive;
#endif
    };

    template<class Derived>
    class basic_iarchive {
    public:
      typedef mpl::false_ is_saving;
      typedef mpl::true_ is_loading;
      template<class T>
      basic_iarchive &operator&(T &t) {
        return that().operator>>(t);
      }
      template<class T>
      basic_iarchive &operator>>(T &t) {
        that().load(t);
        return *this;
      }
      template<class T>
      void load(T &t);
      void load(std::string &s) {
        std::size_t size;
        that() >> size;
        s.resize(size);
        that().load_binary(&*s.begin(), size);  //boost.serialization does the same thing
      }
      void load(std::wstring &s, mpl::false_, mpl::false_ bitwise) {
        std::size_t size;
        that() >> size;
        s.resize(size);
        if (size > 0) that().load_binary(const_cast<wchar_t *>(s.data()), size * sizeof(wchar_t)); //boost.serialization does the same thing
      }
      void load(char *s, mpl::false_, mpl::false_ bitwise) {
        std::size_t size;
        that() >> size;
        if (size > 0) that().load_binary(s, size);
        s[size] = 0;
      }
      void load(wchar_t *s, mpl::false_, mpl::false_ bitwise) {
        std::size_t size;
        that() >> size;
        if (size > 0) that().load_binary(s, size * sizeof(wchar_t));
        s[size] = 0;
      }
    private:
      Derived &that() {
        return static_cast<Derived &>(*this);
      }
#ifndef NO_BOOST_SERIALIZATION
    public:
      detail::serialization_iarchive<Derived> &serialization_archive() {
        if (!this->sarchive) {
          this->sarchive = in_place(boost::ref(that()));
        }
        return *this->sarchive;
      }
    private:
      optional<detail::serialization_iarchive<Derived> > sarchive;
#endif
    };

    template<class Derived, class OutputIterator>
    class basic_char_oarchive : public basic_oarchive<Derived> {
    public:
      explicit basic_char_oarchive(OutputIterator const &out) :
          out(out) {
      }
      template<class Size>
      void save_binary(void const *vdata, Size size) {
        this->save_binary(static_cast<char const *>(vdata), size, typename has_array_extension<OutputIterator>::type());
      }
    private:
      template<class Size>
      void save_binary(char const *data, Size size, mpl::true_) {
        this->out = this->out.insert(data, size);
      }
      template<class Size>
      void save_binary(char const *data, Size size, mpl::false_) {
        this->out = std::copy(data, data + size, this->out);
      }

      OutputIterator out;
    };

    template<class Derived, class InputIterator>
    class basic_char_iarchive : public basic_iarchive<Derived> {
    public:
      basic_char_iarchive(InputIterator const &begin, InputIterator const &end) :
          in(begin), end(end) {
      }
      template<class Size>
      void load_binary(void *vdata, Size size) {
        char *data = static_cast<char *>(vdata);
        this->load_binary(data, size, typename has_array_extension<InputIterator>::type());
      }
    private:
      template<class Size>
      void load_binary(char *data, Size size, mpl::true_ arrayex) {
        this->in = this->in.extract(data, size);
      }
      template<class Size>
      void load_binary(char *data, Size size, mpl::false_ arrayex) {
        this->load_binary(data, size, arrayex, typename std::iterator_traits<InputIterator>::iterator_category(),
            typename has_contiguous_values<InputIterator>::type());
      }
      template<class Size>
      void load_binary(char *data, Size size, mpl::false_ arrayex, std::random_access_iterator_tag,
          mpl::true_ contvals) {
        if (std::size_t(this->end - this->in) < size)
          throw archive::archive_exception(archive::archive_exception::input_stream_error);
        std::memcpy(data, &*this->in, size);
        this->in += size;
      }
      template<class Size>
      void load_binary(char *data, Size size, mpl::false_ arrayex, std::random_access_iterator_tag,
          mpl::false_ contvals) {
        //FIXME:
        if (std::size_t(this->end - this->in) < size)
          throw archive::archive_exception(archive::archive_exception::input_stream_error);
        std::copy(data, data + size, this->in);
      }
      template<class Size, class Category>
      void load_binary(char *data, Size size, mpl::false_ arrayex, Category, mpl::false_ contvals) {
        for (std::size_t c = 0; c < size; ++c) {
          if (this->in == this->end) throw archive::archive_exception(archive::archive_exception::input_stream_error);
          *data++ = *this->in++;
        }
      }

      InputIterator in;
      InputIterator end;
    };

    template<class OutputIterator>
    class char_oarchive : public basic_char_oarchive<char_oarchive<OutputIterator>, OutputIterator> {
      typedef basic_char_oarchive<char_oarchive, OutputIterator> base_type;
    public:
      explicit char_oarchive(OutputIterator const &out) :
          base_type(out) {
      }
    };

    template<class InputIterator>
    class char_iarchive : public basic_char_iarchive<char_iarchive<InputIterator>, InputIterator> {
      typedef basic_char_iarchive<char_iarchive<InputIterator>, InputIterator> base_type;
    public:
      char_iarchive(InputIterator const &begin, InputIterator const &end) :
          base_type(begin, end) {
      }
    };

    namespace detail {

      template<class OutputIterator>
      class memory_oarchive : public basic_char_oarchive<memory_oarchive<OutputIterator>, OutputIterator> {
        typedef basic_char_oarchive<memory_oarchive<OutputIterator>, OutputIterator> base_type;
      public:
        explicit memory_oarchive(OutputIterator const &out) :
            base_type(out) {
        }
        void save(std::type_info const *type) {
          this->save(reinterpret_cast<std::size_t>(type));
        }
        using base_type::save;
      };

      template<class InputIterator>
      class memory_iarchive : public basic_char_iarchive<memory_iarchive<InputIterator>, InputIterator> {
        typedef basic_char_iarchive<memory_iarchive<InputIterator>, InputIterator> base_type;
      public:
        memory_iarchive(InputIterator const &begin, InputIterator const &end) :
            base_type(begin, end) {
        }
        void load(std::type_info const *&type) {
          this->load(reinterpret_cast<std::size_t &>(type));
        }
        using base_type::load;
      };

      template<class InputIterator>
      class compare_archive : public basic_oarchive<compare_archive<InputIterator> > {
        typedef basic_oarchive<compare_archive<InputIterator> > base_type;
      public:
        compare_archive(InputIterator const &begin, InputIterator const &end) :
            in(begin), end(end), equal_(true) {
          BOOST_ASSERT(end - begin >= 0);
        }
        template<class Size>
        void save_binary(void const *vdata, Size size) {
          char const *data = static_cast<char const *>(vdata);
          this->save_binary(data, size, typename std::iterator_traits<InputIterator>::iterator_category(),
              typename has_contiguous_values<InputIterator>::type());
        }
        void save(std::type_info const *type) {
          this->save(reinterpret_cast<std::size_t>(type));
        }
        template<class T>
        void save(T const &t) {
          if (this->equal_) { //not equal()!
            base_type::save(t);
          }
        }
        bool equal() const {
          return this->equal_ && this->in == this->end;
        }
      private:
        template<class Size>
        void save_binary(char const *data, Size size, std::random_access_iterator_tag, mpl::true_ contvals) {
          if ((std::size_t(this->end - this->in) < size) || (std::memcmp(data, &*this->in, size) != 0)) this->equal_ =
              false;
          else this->in += size;
        }
        template<class Size>
        void save_binary(char const *data, Size size, std::random_access_iterator_tag, mpl::false_ contvals) {
          //FIXME: invalid iterator:
          if ((std::size_t(this->end - this->in) < size) || (!std::equal(data, data + size, this->in))) this->equal_ =
              false;
          else this->in += size;
        }
        template<class Size, class Category>
        void save_binary(char const *data, Size size, Category, mpl::false_ contvals) {
          for (std::size_t c = 0; c < size; ++c) {
            if ((this->in == this->end) || (*data++ != *this->in++)) {
              this->equal_ = false;
              break;
            }
          }
        }

        InputIterator in;
        InputIterator end;
        bool equal_;
      };

      template<class UnaryFunction>
      class apply_archive : public basic_oarchive<apply_archive<UnaryFunction> > {
        typedef basic_oarchive<apply_archive<UnaryFunction> > base_type;
      public:
        explicit apply_archive(UnaryFunction const &f) :
            f(f) {
        }
        void save_binary(void const *, std::size_t) {
        }
        template<class T>
        void save(T const &t);
        using base_type::save;

        template<class T>
        void operator()(T const &t) const {
          this->f(t);
        }
      private:
        UnaryFunction f;
      };

    }

  }
}

#include <boost/transact/object_access.hpp>

template<class Derived>
template<class T>
void boost::transact::basic_oarchive<Derived>::save(T const &t) {
  object_access::save(that(), t);
}

template<class Derived>
template<class T>
void boost::transact::basic_iarchive<Derived>::load(T &t) {
  object_access::load(that(), t);
}

template<class UnaryFunction>
template<class T>
void boost::transact::detail::apply_archive<UnaryFunction>::save(T const &t) {
  this->f(t);
  object_access::apply(*this, t);
}

#endif

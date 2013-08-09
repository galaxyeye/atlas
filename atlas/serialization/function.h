/*
 * function.h
 *
 *  Created on: Jan 29, 2013
 *      Author: vincent ivincent.zhang@gmail.com
 */

#ifndef ATLAS_FUNCTION_H_
#define ATLAS_FUNCTION_H_

#include <cstddef> // std::nullptr
#include <memory>
#include <tuple>
#include <functional> // std::function
#include <utility> // std::move
#include <type_traits> // std::decay, and others

#include <atlas/serialization/tuple.h>
#include <atlas/apply_tuple.h>

namespace atlas {
  namespace serialization {

    namespace { struct useless { }; }

    template<typename Signature> class function;

    template<typename Res, typename ... Args>
    class function<Res(Args...)> {
    public:

      typedef Res result_type;

      function() = default;

      function(std::nullptr_t) {}

      function(const function& other) : _args(other._args), _f(other._f) { }

      function(function&& other) : _args(std::move(other._args)), _f(std::move(other._f)) {}

      template<typename Archive, typename Functor>
      function(Functor f, Args... args, Archive& ar,
          typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless())
      : _args(args...), _f(f)
      {
        ar & _args;
      }

      /*
       * The last parameter can be replaced by a local variable
       * */
      template<typename Functor, typename IArchiver>
      function(Functor f, IArchiver& ia,
          typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless()) :
          _f(f)
      {
        ia >> _args;
      }

      /**
       *  @brief %function assignment operator.
       *  @param other A %function with identical call signature.
       *  @post @c (bool)*this == (bool)x
       *  @returns @c *this
       *
       *  The target of @a other is copied to @c *this. If @a other has no
       *  target, then @c *this will be empty.
       *
       *  If @a other targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      function& operator=(const function& other) {
        function(other).swap(*this);
        return *this;
      }

      /**
       *  @brief %function move-assignment operator.
       *  @param other A %function rvalue with identical call signature.
       *  @returns @c *this
       *
       *  The target of @a other is moved to @c *this. If @a other has no
       *  target, then @c *this will be empty.
       *
       *  If @a other targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      function& operator=(function&& other) {
        function(std::move(other)).swap(*this);
        return *this;
      }

      /**
       *  @brief %function assignment to zero.
       *  @returns @c *this
       */
      function& operator=(std::nullptr_t) {
        function().swap(*this);

        return *this;
      }

      /**
       *  @brief %function assignment to a new target.
       *  @param f A %function object that is callable with parameters of
       *  type @c T1, @c T2, ..., @c TN and returns a value convertible
       *  to @c Res.
       *  @return @c *this
       *
       *  This  %function object wrapper will target a copy of @a
       *  f. If @a f is @c std::reference_wrapper<F>, then this function
       *  object will contain a reference to the function object @c
       *  f.get(). If @a f is a NULL function pointer or NULL
       *  pointer-to-member, @c this object will be empty.
       *
       *  If @a f is a non-NULL function pointer or an object of type @c
       *  std::reference_wrapper<F>, this function will not throw.
       */
      template<typename Functor>
      typename std::enable_if<!std::is_integral<Functor>::value, function&>::type
      operator=(Functor&& f) {
        function(std::forward<Functor>(f)).swap(*this);
        return *this;
      }

      template<typename Functor>
      typename std::enable_if<!std::is_integral<Functor>::value, function&>::type
      operator=(std::reference_wrapper<Functor> f) {
        function(f).swap(*this);
        return *this;
      }

      /**
       *  @brief Swap the targets of two %function objects.
       *  @param other A %function with identical call signature.
       *
       *  Swap the targets of @c this function object and @a f. This
       *  function will not throw an %exception.
       */
      void swap(function& other) {
        std::swap(_args, other._args);
        std::swap(_f, other._f);
      }

      /**
       *  @brief Determine if the %function wrapper has a target.
       *
       *  @return @c true when this %function object contains a target,
       *  or @c false when it is empty.
       *
       *  This function will not throw an %exception.
       */
      explicit operator bool() const
      { return _f.operator bool();}

      // [3.7.2.4] function invocation

      /**
       *  @brief Invokes the function targeted by @c *this.
       *  @returns the result of the target.
       *  @throws bad_function_call when @c !(bool)*this
       *
       *  The function call operator invokes the target function object
       *  stored by @c this.
       */
      Res operator()() const { return apply_tuple(_f, _args); }

    public:

      std::tuple<typename std::decay<Args>::type...> _args;
      std::function<Res(Args...)> _f;
    };

  }
} // atlas

// i hope so
//namespace boost {
//  namespace serialization {
//
//    template <typename Archive, typename Res, typename... Args>
//    void serialize(Archive& ar, std::function<Res(Args...)>& fun, const unsigned int /* version */) {
//      ar & boost::serialization::make_nvp("function", fun);
//    }
//
//  } // serialization
//} // boost

#endif /* ATLAS_FUNCTION_H_ */

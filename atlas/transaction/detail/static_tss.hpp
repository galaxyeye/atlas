//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_TRANSACT_DETAIL_STATIC_TSS_HPP
#define BOOST_TRANSACT_DETAIL_STATIC_TSS_HPP

#include <boost/noncopyable.hpp>
#include <boost/thread/tss.hpp>

namespace boost{
namespace transact{
namespace detail{

template<class T,class Tag,bool Threads=true>
class static_thread_specific_ptr;


template<class T,class Tag>
class static_thread_specific_ptr<T,Tag,true> : noncopyable{
private:
    static_thread_specific_ptr();
public:

#if (__GNUC__== 3 && __GNUC_MINOR__ > 4) || __GNUC__>= 4
    static void reset(T *p=0){ ptr=p; }
    static T *get(){ return ptr; }
private:
    static __thread T *ptr;
};

template<class T,class Tag>
__thread T *static_thread_specific_ptr<T,Tag,true>::ptr(0);

#elif defined(BOOST_MSVC)
    static void reset(T *p=0){ ptr=p; }
    static T *get(){ return ptr; }
private:
    static __declspec(thread) T *ptr;
};

template<class T,class Tag>
__declspec(thread) T *static_thread_specific_ptr<T,Tag,true>::ptr(0);


#else
    static void reset(T *p=0){ ptr.reset(p); }
    static T *get(){ return ptr.get(); }
private:
    static thread_specific_ptr<T> ptr;
};

template<class T,class Tag>
thread_specific_ptr<T> static_thread_specific_ptr<T,Tag,true>::ptr(0); //null deleter

#endif

template<class T,class Tag>
class static_thread_specific_ptr<T,Tag,false> : noncopyable{
private:
    static_thread_specific_ptr();
public:
    static void reset(T *p=0){ ptr=p; }
    static T *get(){ return ptr; }
private:
    static T *ptr;
};

template<class T,class Tag>
T *static_thread_specific_ptr<T,Tag,false>::ptr(0);

}
}
}


#endif

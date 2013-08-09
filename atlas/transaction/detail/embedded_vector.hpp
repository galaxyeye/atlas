//          Copyright Stefan Strasser 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_TRANSACT_DETAIL_EMBEDDED_VECTOR_HEADER_HPP
#define BOOST_TRANSACT_DETAIL_EMBEDDED_VECTOR_HEADER_HPP

#include <iterator>
#include <algorithm>
#include <cstring>
#include <boost/utility/in_place_factory.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/assert.hpp>
#include <boost/transact/array_extension.hpp>


namespace boost{
namespace transact{
namespace detail{

template<class T,std::size_t EmbeddedSize,bool Expand=true>
class embedded_vector{
public:
    typedef T &reference;
    typedef T const &const_reference;
    typedef T *iterator; //continuous values
    typedef T const *const_iterator;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T const *const_pointer;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    embedded_vector()
        : begin_(emb_data())
        , end_(emb_data())
        , end_storage(emb_data() + EmbeddedSize){}

    embedded_vector(size_type n,T const &value=T());
    template<class InputIterator>
    embedded_vector(InputIterator begin,InputIterator end);
    embedded_vector(embedded_vector const &);
    ~embedded_vector(){
        destruct(this->begin(),this->end());
        if(Expand){
            if(this->begin() != this->emb_data()){
                ::operator delete(this->begin_);
            }
        }else BOOST_ASSERT(this->begin() == this->emb_data());
    }
    embedded_vector &operator=(embedded_vector const &);
    iterator begin(){ return this->begin_; }
    const_iterator begin() const{ return this->begin_; }
    iterator end(){ return this->end_; }
    const_iterator end() const{ return this->end_; }
    reverse_iterator rbegin(){ return reverse_iterator(this->end()); }
    const_reverse_iterator rbegin() const{ return const_reverse_iterator(this->end()); }
    reverse_iterator rend(){ return reverse_iterator(this->begin()); }
    const_reverse_iterator rend() const{ return const_reverse_iterator(this->begin()); }
    size_type size() const{
        return this->end() - this->begin();
    }
    size_type max_size() const{
        if(Expand) return size_type(-1);
        else return EmbeddedSize;
    }
    void resize(size_type s,T const &c=T()){
        this->reserve(s);
        if(s > this->size()) copy_construct(this->end(),this->begin()+s,c);
        else destruct(this->begin()+s,this->end());
        this->end_=this->begin_+s;
    }
    size_type capacity() const{
        return this->end_storage - this->begin_;
    }
    bool empty() const{
        return this->begin() == this->end();
    }
    void reserve(size_type mincap){
        this->reserve(mincap,mpl::bool_<Expand>());
    }
    reference operator[](size_type n){
        BOOST_ASSERT(n < this->size());
        return *(this->begin() + n);
    }
    const_reference operator[](size_type n) const{
        BOOST_ASSERT(n < this->size());
        return *(this->begin() + n);
    }
    reference at(size_type);
    const_reference at(size_type) const;
    
    reference front(){ return *this->begin(); }
    const_reference front() const{ return *this->begin(); }
    reference back(){ return *this->rbegin(); }
    const_reference back() const{ return *this->rbegin(); }
    
    template<class InputIterator>
    void assign(InputIterator begin,InputIterator end);
    void assign(size_type n,T const &u);
    
    void push_back(T const &x){
        this->reserve_add(mpl::size_t<1>());
        new (this->end_) T(x);
        ++this->end_;
    }

    //extension to efficiently insert arrays.
    //insert(this->end(), ...) is not the same thing. even though the end() == end()
    //comparison can be optimized away if inlined, size must be statically known to get
    //an intrinsic memcpy for PODs.
    template<class InputIterator,class Size>
    void push_back(InputIterator src,Size size){
        this->reserve_add(size);
        copy_construct_n(this->end_,src,size);
        this->end_+=size;
    }

    //extension to in-place construct a new element. elements of vectors with Expand==false
    //don't need to be copyconstructible using this: FIXME use c++11
    template<class InPlaceFactory>
    void push_back(InPlaceFactory const &fac){
        this->reserve_add(mpl::size_t<1>());
        fac.template apply<T>(this->end_);
        ++this->end_;
    }

    void pop_back();
    iterator insert(iterator position,T const &x);
    void insert(iterator position,size_type n,T const &x);
    template<class InputIterator>
    void insert(iterator position,InputIterator first,InputIterator last);
    iterator erase(iterator position);
    iterator erase(iterator begin,iterator end);
    void swap(embedded_vector &);
    void clear(){
        destruct(this->begin(),this->end());
        this->end_=this->begin();
    }
private:
    static void copy_construct(T *dest,T const *begin,T const *end){
        copy_construct_n(dest,begin,std::size_t(end-begin));
    }
    static void copy_construct(T *begin,T *end,T const &x){
        static bool const usememset=boost::is_pod<T>::value && sizeof(T) == 1;
        copy_construct(begin,end,x,mpl::bool_<usememset>());
    }
    static void copy_construct(T *begin,T *end,T const &x,mpl::true_ usememset){
        std::memset(begin,x,end-begin);
    }
    static void copy_construct(T *begin,T *end,T const &x,mpl::false_ usememset){
        T *it;
        try{
            for(it=begin;it != end;++it){
                new (it) T(x);
            }
        }catch(...){
            destruct(begin,it);
            throw;
        }
    }
    template<class InputIterator,class Size>
    static void copy_construct_n(T *dest,InputIterator src,Size n){
        copy_construct_n(
            dest,src,n,
            mpl::bool_<is_pod<T>::value && has_contiguous_values<InputIterator>::value>()
	);
    }
    template<class InputIterator,class Size>
    static void copy_construct_n(T *dest,InputIterator src,Size n,true_type pod){
        std::memcpy(dest,&*src,sizeof(T)*std::size_t(n));
    }
    template<class InputIterator,class Size>
    static void copy_construct_n(T *dest,InputIterator src,Size n,false_type pod){
        std::size_t c;
        try{
            for(c=0;c<n;++c){
                new (dest+c) T(*src++);
            }
        }catch(...){
            destruct(dest,dest+c);
            throw;
        }
    }
    static void destruct(T const *begin,T const *end){
        destruct(begin,end,typename is_pod<T>::type());
    }
    static void destruct(T const *begin,T const *end,true_type pod){}
    static void destruct(T const *begin,T const *end,false_type pod){
        for(T const *it=begin;it != end;++it){
            it->~T(); //nothrow
        }
    }
    //equivalent to reserve(size() + s)
    template<class Size>
    void reserve_add(Size s){
        this->reserve_add(s,mpl::bool_<Expand>());
    }
    template<class Size>
    void reserve_add(Size s,mpl::true_ expand){
        if(this->end_ + s > this->end_storage) this->reallocate(this->size()+s);
    }
    template<class Size>
    void reserve_add(Size s,mpl::false_ expand){
        BOOST_ASSERT(this->capacity() >= this->size() + s);
    }

    void reserve(size_type mincap,mpl::true_ expand){
        if(this->capacity() < mincap) this->reallocate(mincap);
    }
    void reserve(size_type mincap,mpl::false_ expand){
        BOOST_ASSERT(this->capacity() >= mincap);
    }
    void reallocate(size_type mincap){
        BOOST_ASSERT(mincap > this->capacity());
        size_type newcap=this->capacity() * 2;
        if(newcap < mincap) newcap=mincap;
        void *newdata=::operator new(newcap * sizeof(T));
        T *newbegin=reinterpret_cast<T *>(newdata);
        try{
            copy_construct(newbegin,this->begin(),this->end());
            destruct(this->begin(),this->end());
        }catch(...){ ::operator delete(newdata); throw; }
        this->end_=newbegin + this->size();
        this->end_storage=newbegin + newcap;
        if(this->begin() != this->emb_data()) ::operator delete(this->begin_);
        this->begin_=newbegin;
    }
    T *emb_data(){
        return reinterpret_cast<T *>(this->emb_data_);
    }

    char emb_data_[sizeof(T)*EmbeddedSize];
    T *begin_; //TODO optimization: begin_ and end_storage are not needed if Expand==false
    T *end_;
    T *end_storage;
};


}

template<class T,std::size_t EmbeddedSize,bool Expand>
struct has_array_extension<detail::embedded_vector<T,EmbeddedSize,Expand> > : mpl::true_{};

}
}


#endif

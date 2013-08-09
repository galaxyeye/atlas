//          Copyright Stefan Strasser 2010 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_ARRAY_EXTENSION_HPP
#define BOOST_TRANSACT_ARRAY_EXTENSION_HPP

#include <boost/mpl/bool.hpp>
#include <iterator>

namespace boost{
namespace transact{

template<class T>
struct has_array_extension : mpl::false_{};

template<class T>
struct has_contiguous_values : mpl::false_{};

template<class T>
struct has_contiguous_values<T *> : mpl::true_{};


template<typename Vector>
class vector_back_insert_iterator
    : public std::iterator<std::output_iterator_tag,typename Vector::value_type,typename Vector::difference_type,typename Vector::pointer_type,typename Vector::reference_type>{
public:
    typedef Vector container_type;
    explicit vector_back_insert_iterator(Vector &vec) : vec(&vec){}
    vector_back_insert_iterator &operator=(typename Vector::const_reference v){
        this->vec->push_back(v);
        return *this;
    }
    template<class InputIterator,class Size>
    vector_back_insert_iterator &insert(InputIterator input,Size size){
        this->insert(input,size,typename has_array_extension<Vector>::type());
	return *this;
    }
    vector_back_insert_iterator &operator*(){ return *this; }
    vector_back_insert_iterator &operator++(){ return *this; }
    vector_back_insert_iterator operator++(int){ return *this; }
private:
    template<class InputIterator,class Size>
    void insert(InputIterator input,Size size,mpl::true_ arrayex){
        this->vec->push_back(input,size);
    }
    template<class InputIterator,class Size>
    void insert(InputIterator input,Size size,mpl::false_ arrayex){
        this->insert(input,size,arrayex,
            mpl::bool_<has_contiguous_values<InputIterator>::value
                       && is_pod<typename Vector::value_type>::value>());
    }
            
    template<class InputIterator,class Size>
    void insert(InputIterator input,Size size,mpl::false_ arrayex,mpl::true_ contigpod){
        std::size_t const oldsize=this->vec->size();
        this->vec->resize(oldsize + size);
        std::memcpy(&(*this->vec)[oldsize],&*input,size * sizeof(typename Vector::value_type));
    }
    template<class InputIterator,class Size>
    void insert(InputIterator input,Size size,mpl::false_ arrayex,mpl::false_ contigpod){
        this->vec->reserve(this->vec->size() + size);
	for(std::size_t c=0;c<size;++c){
            this->vec->push_back(*input++);
	}
    }

    Vector *vec;
};


template<class Vector>
struct has_array_extension<vector_back_insert_iterator<Vector> > : mpl::true_{};



}
}



#endif

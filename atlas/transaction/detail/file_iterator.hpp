//               Copyright Stefan Strasser 2013
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_FILE_ITERATOR_HPP
#define BOOST_TRANSACT_DETAIL_FILE_ITERATOR_HPP


#include <iterator>
#include <boost/mpl/bool.hpp>
#include <boost/transact/array_extension.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/static_assert.hpp>
#include <boost/optional/optional.hpp>


namespace boost{
namespace transact{
namespace detail{

template<typename File>
class file_output_iterator
    : public std::iterator<std::output_iterator_tag,char>{
public:
    typedef File file_type;
    explicit file_output_iterator(File &file) : file(&file){}
    file_output_iterator &operator=(char v){
        this->file->write(&v,mpl::size_t<sizeof(char)>());
        return *this;
    }    
    template<class InputIterator,class Size>
    file_output_iterator &insert(InputIterator input,Size size){
        this->insert(input,size,typename has_contiguous_values<InputIterator>::type());
	return *this;
    }
    file_output_iterator &operator*(){ return *this; }
    file_output_iterator &operator++(){ return *this; }
    file_output_iterator operator++(int);
private:
    template<class InputIterator,class Size>
    void insert(InputIterator input,Size size,mpl::true_ contig){
	this->file->write(&*input,size);
    }
    template<class InputIterator,class Size>
    void insert(InputIterator input,Size size,mpl::false_ contig){
	for(std::size_t c=0;c<size;++c){
            this->file->write(&*input,mpl::size_t<sizeof(char)>());
	    ++input;
	}
    }

    File *file;
};

template<class File>
class file_input_iterator
    : std::iterator<std::forward_iterator_tag,char>{
public:
    typedef File file_type;
    file_input_iterator() : file(0){} //eof iterator  
    explicit file_input_iterator(File &file) : file(&file){}
    char operator*() const;
    file_input_iterator &operator++();
    file_input_iterator operator++(int);
    bool operator==(file_input_iterator const &other) const;
    bool operator!=(file_input_iterator const &other) const;
    template<class OutputIterator,class Size>
    file_input_iterator &extract(OutputIterator output,Size size){
        this->extract(output,size,typename has_contiguous_values<OutputIterator>::type());
	return *this;
    }
private:
    template<class OutputIterator,class Size>
    void extract(OutputIterator output,Size size,mpl::true_ contig){
	this->file->read(&*output,size);
    }
    template<class OutputIterator,class Size>
    void extract(OutputIterator output,Size size,mpl::false_ contig){
	for(std::size_t c=0;c<size;++c){
            this->file->read(&*output,mpl::size_t<sizeof(char)>());
	    ++output;
	}
    }
    File *file;  
};

  
}

template<class File>
struct has_array_extension<detail::file_output_iterator<File> > : mpl::true_{};
template<class File>
struct has_array_extension<detail::file_input_iterator<File> > : mpl::true_{};


}
}

#endif

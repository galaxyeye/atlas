//          Copyright Stefan Strasser 2009 - 2013.
//                Copyright Bob Walters 2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_BUFFERING_FILE_HPP
#define BOOST_TRANSACT_DETAIL_BUFFERING_FILE_HPP

#include <boost/mpl/size_t.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost {
  namespace transact {
    namespace detail {

      template<std::size_t Capacity, bool direct_io>
      struct ofile_buffer {
        static const bool direct = direct_io;
        char data[Capacity];
      };

#ifdef _WIN32
// Direct I/O supported on Windows given that there are only two ways
// to achieve synchronized sequaltial disk I/O - flush the system I/O buffers
// or use Windows direct I/O.  Not supported on any other OS.  Although O_DIRECT
// supported on many, strong
      template<std::size_t Capacity>
      struct ofile_buffer<Capacity,true> {
        static const bool direct = true;
        char *data;

        ofile_buffer() {
          // direct I/O requires pagesize alignment.  This assumes
          // Capacity is > pagesize, but not necessarilly a multiple of it.
          int alignment=1;// largest power of 2 >= Capacity
          for (std::size_t i=Capacity; (i>>=1); alignment<<=1 )
          ;
          data = (char*)_aligned_malloc(Capacity, alignment);
        };
        ~ofile_buffer() {
          _aligned_free(data);
        };
      };
#endif

      template<class Base, std::size_t Capacity>
      class buffering_seq_ofile {
      public:

        typedef typename Base::size_type size_type;

        static const bool has_direct_io = Base::has_direct_io;

        explicit buffering_seq_ofile(std::string const &name) :
            base(name), size(0) {
        }

        template<class Size>
        void write(void const *data, Size s) {
          if (this->size + s <= Capacity) {
            std::memcpy(this->buffer.data + this->size, data, s);
            this->size += s;
          }
          else this->write_overflow(data, s);
        }

        size_type position() const {
          return this->base.position() + this->size;
        }

        void flush() {
          this->flush_buffer();
          this->base.flush();
        }

        void close() {
          this->flush_buffer();
          this->base.close();
        }

        void reopen(std::string const &name) {
          BOOST_ASSERT(this->size == 0);
          this->base.reopen(name);
        }

        void sync() {
          //don't flush buffer! caller is responsible to call flush() inside a mutex lock.
          this->base.sync();
        }

        ~buffering_seq_ofile() {
          try {
            this->flush_buffer();
          }
          catch (...) {
#ifndef NDEBUG
            std::cerr << "ignored exception" << std::endl;
#endif
          }
        }

      private:

        void write_overflow(void const *data, std::size_t s) {
          BOOST_ASSERT(this->size + s > Capacity);
          if (has_direct_io) {
            while (this->size + s > Capacity) {
              std::size_t write = Capacity - this->size;
              std::memcpy(this->buffer.data + this->size, data, write);
              this->size = Capacity;
              this->flush_full_buffer();
              data = static_cast<char const *>(data) + write;
              s -= write;
            }
            if (s) { //FIXME ???
              this->write(data, s);
            }
          }
          else if (this->size == 0) {
            this->base.write(data, s);
          }
          else {
            std::size_t write = Capacity - this->size;
            std::memcpy(this->buffer.data + this->size, data, write);
            this->size = Capacity;
            this->flush_full_buffer();
            this->write(static_cast<char const *>(data) + write, s - write);
          }
        }

        void flush_buffer() {
          if (this->size > 0) {
            this->base.write(this->buffer.data, this->size);
            this->size = 0;
          }
        }

        void flush_full_buffer() {
          BOOST_ASSERT(this->size == Capacity);
          this->base.write(this->buffer.data, mpl::size_t<Capacity>());
          this->size = 0;
        }

        Base base;
        ofile_buffer<Capacity, has_direct_io> buffer;
        std::size_t size;
      };

    }
  }
}

#endif

//          Copyright Stefan Strasser 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_SECTORIZING_FILE_HPP
#define BOOST_TRANSACT_DETAIL_SECTORIZING_FILE_HPP

#include <boost/mpl/size_t.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <cstring>
#include <boost/transact/detail/file.hpp>

namespace boost {
  namespace transact {
    namespace detail {

      template<class Base>
      class sectorizing_seq_ofile {
      public:

        typedef typename Base::size_type size_type;

        explicit sectorizing_seq_ofile(std::string const &name) :
            base(name), size(0) {
        }

        template<class Size>
        void write(void const *data, Size s) {
          if (this->size + s <= max_size) {
            std::memcpy(this->buffer(this->size), data, s);
            this->size += s;
          }
          else this->write_overflow(data, s);
        }

        size_type position() const { return this->base.position() + this->size + 1; }

        void flush() {
          this->flush_buffer();
          this->base.flush();
        }

        void sync() { this->base.sync(); }

        ~sectorizing_seq_ofile() {
          try {
            this->flush_buffer();
          }
          catch (...) {
#ifndef NDEBUG
            std::cerr << "ignored exception" << std::endl;
#endif
          }
        }

        void close() {
          this->flush_buffer();
          this->base.close();
        }

        void reopen(std::string const &name) {
          BOOST_ASSERT(this->size == 0);
          this->base.reopen(name);
        }

      private:

        static std::size_t const sector_size = 512;
        static std::size_t const max_size = sector_size - 2;

        void write_overflow(void const *data, std::size_t s) {
          BOOST_ASSERT(this->size + s > max_size);

          std::size_t write = max_size - this->size;
          std::memcpy(this->buffer(this->size), data, write);
          this->size = max_size;
          std::size_t left = s - write;
          char const *cdata = static_cast<char const *>(data) + write;
          while (left > 0) {
            this->flush_buffer();
            BOOST_ASSERT(this->size == 0);
            if (left <= max_size) write = left;
            else write = max_size;
            std::memcpy(this->buffer(this->size), cdata, write);
            this->size += write;
            left -= write;
            cdata += write;
          }
        }

        void flush_buffer() {
          if (this->size > 0) {
            BOOST_STATIC_ASSERT(max_size < (1 << 14));
            BOOST_ASSERT(this->size <= max_size);
            this->buffer_[0] = this->size | 0x80;
            this->buffer_[sector_size - 1] = (this->size >> 7) | 0x80;
            this->base.write(this->buffer_, mpl::size_t<sector_size>());
            this->size = 0;
          }
        }

        char *buffer(std::size_t i) {
          return this->buffer_ + i + 1;
        }

        Base base;
        char buffer_[sector_size];
        std::size_t size;
      };

      template<class Base>
      class sectorizing_seq_ifile {
      public:

        typedef typename Base::size_type size_type;

        explicit sectorizing_seq_ifile(std::string const &name) :
            base(name), pos(0), size(0) {
        }

        template<class Size>
        void read(void *data, Size s) {
          if (this->size == 0 || this->pos == this->size) this->read_sector();
          if (this->pos + s <= this->size) {
            std::memcpy(data, this->buffer(this->pos), s);
            this->pos += s;
          }
          else {
            try {
              std::size_t read = this->size - this->pos;
              BOOST_ASSERT(read > 0);
              std::memcpy(data, this->buffer(this->pos), read);
              this->pos += read;
              std::size_t left = s - read;
              char *cdata = static_cast<char *>(data) + read;
              while (left > 0) {
                this->read_sector();
                if (left <= this->size) read = left;
                else read = this->size;
                std::memcpy(cdata, this->buffer(this->pos), read);
                this->pos += read;
                left -= read;
                cdata += read;
              }
            }
            catch (eof_exception &) {
              //at least some data was read, real EOFs are thrown above
              throw io_failure();
            }
          }
        }

        size_type position() const;

      private:

        void read_sector() {
          BOOST_ASSERT(this->pos == this->size);
          do {
            this->base.read(this->buffer_, mpl::size_t<sector_size>());
            if (this->buffer_[0] == 0 || this->buffer_[sector_size - 1] == 0) throw eof_exception();
            this->size = ((this->buffer_[sector_size - 1] & 0x7f) << 7) | (this->buffer_[0] & 0x7f);
            if (this->size > max_size) throw io_failure();
          }
          while (this->size == 0);
          this->pos = 0;
        }

        char *buffer(std::size_t p) {
          return this->buffer_ + p + data_begin;
        }

        static std::size_t const sector_size = 512;
        static std::size_t const data_begin = 1;
        static std::size_t const data_end = sector_size - 1;
        static std::size_t const max_size = data_end - data_begin;

        Base base;
        char buffer_[sector_size];
        std::size_t pos;
        std::size_t size;
      };

    }
  }
}

#endif

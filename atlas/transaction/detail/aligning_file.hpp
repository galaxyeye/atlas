//          Copyright Stefan Strasser 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_ALIGNING_FILE_HPP
#define BOOST_TRANSACT_DETAIL_ALIGNING_FILE_HPP

#include <boost/mpl/size_t.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost {
  namespace transact {
    namespace detail {

//linux needs sync intervals up to 4096 bytes to be aligned,
//e.g., starting at position 0, and | representing a sync:

//512 | 2048 | ...

//does not result in sequential writing performance, instead:

//512 | [1536] | 2048 | ...

//does. the inserted 1536 bytes of garbage even need to be synced.

//TODO optimization: not performance tested on windows.

      template<class Base>
      class aligning_seq_ofile {
      public:

        typedef typename Base::size_type size_type;

      private:

        static std::size_t const sector_size = 512;

        static std::size_t const max_alignment = 4096;

        static std::size_t const max_sectors = max_alignment / sector_size / 2;

      public:

        explicit aligning_seq_ofile(std::string const &name) :
            base(name), sectors(0) {
        }

        void write(void const *data, mpl::size_t<sector_size> size) {
          if (this->sectors < max_sectors) {
            std::memcpy(this->buffer + this->sectors * sector_size, data, size);
            ++this->sectors;
          }
          else this->write_overflow(data);
        }

        size_type position() const {
          if (this->sectors <= max_sectors) return this->base.position() + this->sectors * sector_size;
          else return this->base.position();
        }

        void close() {
          this->flush_buffer();
          this->sectors = 0;
          this->base.close();
        }

        void reopen(std::string const &name) {
          BOOST_ASSERT(this->sectors == 0);
          this->base.reopen(name);
        }

        void flush() {
          BOOST_ASSERT(this->base.position() % sector_size == 0);
          if (this->sectors > 1 && this->sectors <= max_sectors) {
            BOOST_STATIC_ASSERT(max_sectors == 4);
            this->align((this->sectors == 3 ? 4 : this->sectors) * sector_size);
          }
          this->flush_buffer();
          this->sectors = 0;
          this->base.flush();
        }

        void sync() {
          this->base.sync();
        }

        ~aligning_seq_ofile() {
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

        void write_overflow(void const *data) {
          BOOST_ASSERT(this->sectors >= max_sectors);
          if (this->sectors == max_sectors) {
            this->align(max_alignment);
            this->flush_buffer();
            this->sectors = max_sectors + 1;
          }
          this->base.write(data, sector_size);
        }

        void flush_buffer() {
          if (this->sectors > 0 && this->sectors <= max_sectors) {
            this->base.write(this->buffer, this->sectors * sector_size);
          }
        }

        void align(std::size_t alignment) {
          BOOST_ASSERT(this->base.position() % sector_size == 0);
          std::size_t mod = this->base.position() % alignment;
          if (mod != 0) {
            std::size_t write = alignment - mod;
            BOOST_ASSERT(write <= empty_sectors_t::size && write % sector_size == 0);
            this->base.write(empty_sectors.data, write);
            this->base.flush();
            //this sync is unnecessary from a data-consistency viewpoint.
            //but it is required to keep linux is sequential writing.
            this->base.sync(); //TODO optimization: sync inside mutex lock
          }
        }

        Base base;
        unsigned char buffer[max_sectors * sector_size];
        std::size_t sectors;

        struct empty_sectors_t {
          empty_sectors_t() {
            for (std::size_t c = 0; c < sectors; ++c)
              clear_sector(this->data + c * sector_size);
          }

          static std::size_t const size = max_alignment - sector_size;

          static std::size_t const sectors = size / sector_size;

          unsigned char data[size];

          static void clear_sector(unsigned char *sec) {
            sec[0] = 0x80;
            sec[sector_size - 1] = 0x80;
          }
        };

        static empty_sectors_t empty_sectors;
      };

      template<class Base>
      typename aligning_seq_ofile<Base>::empty_sectors_t aligning_seq_ofile<Base>::empty_sectors;

    }
  }
}

#endif

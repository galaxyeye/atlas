//          Copyright Stefan Strasser 2009 - 2013
//                Copyright Bob Walters 2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_SYNCING_FILE_HPP
#define BOOST_TRANSACT_DETAIL_SYNCING_FILE_HPP

#include <string>
#include <cstring>
#include <boost/filesystem.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/transact/detail/buffering_file.hpp>

namespace boost {
  namespace transact {
    namespace detail {

      static std::size_t const file_write_ahead_size = 10 * 1024 * 1024;
      static std::size_t const file_page_size = 4096;

      template<class Base>
      class syncing_seq_ofile {
      public:

        typedef unsigned int size_type;

        static const bool has_direct_io = Base::has_direct_io;

        explicit syncing_seq_ofile(std::string const &name) :
            pos(0), base(name) {
          this->write_ahead(0, file_write_ahead_size);
        }

        void write(void const *data, std::size_t size) {
          size_type const s = this->pos % file_write_ahead_size;
          if (s + size >= file_write_ahead_size) {
            // there must be at least one 0 at the and, so also write ahead if this is equal.
            size_type start = this->pos - s + file_write_ahead_size;
            // usually == start + file_write_ahead_size, but "size" can theoretically span a whole file_write_ahead_size
            size_type end = start + ((s + size) / file_write_ahead_size) * file_write_ahead_size;
            BOOST_ASSERT(end > start);
            this->write_ahead(start, end);
          }

          std::size_t ret = base.write((char const *) data, size);
          if (ret > 0) this->pos += ret;
          if (ret != std::size_t(size)) throw io_failure();
        }

        size_type position() const {
          return this->pos;
        }

        void flush() {
          base.flush();
        }

        void sync() {
          base.sync();
        }

        void close() {
          //do not actually close, just make sure contents are on disk. no call to base.close()!
          this->flush();
          this->sync(); //TODO opimize sync in lock?
          this->pos = 0;
        }

        void reopen(std::string const &name) {
          BOOST_ASSERT(this->pos == 0);
          //file is still open, invalidate file for reuse BEFORE renaming:
          this->write_ahead(0, file_write_ahead_size);
          this->base.close();
          this->base.rename(name); //rename, not reopen! by writing ahead the file is effectively truncated
        }

      private:

        size_type pos;
        Base base;

        void write_ahead(size_type const &start, size_type const &end) {
          BOOST_ASSERT(start % file_write_ahead_size == 0);
          BOOST_ASSERT(end % file_write_ahead_size == 0);
          BOOST_STATIC_ASSERT(file_write_ahead_size % file_page_size == 0);
          base.seek(start);
          for (size_type off = start; off < end; off += file_page_size) {
            base.write(empty_page.data, file_page_size);
          }
          this->flush();
          this->sync();
          base.seek(this->pos);
        }

        struct empty_page_type : public ofile_buffer<file_page_size, has_direct_io> {
          typedef ofile_buffer<file_page_size, has_direct_io> base_buffer;
          empty_page_type() :
              base_buffer() {
            std::memset(base_buffer::data, 0, file_page_size);
          }
        };

        static empty_page_type empty_page;
      };

      template<class Base>
      typename syncing_seq_ofile<Base>::empty_page_type syncing_seq_ofile<Base>::empty_page;

    }
  }
}

#endif

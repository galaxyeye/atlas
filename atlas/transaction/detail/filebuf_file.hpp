//          Copyright Stefan Strasser 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_FILEBUF_FILE_HPP
#define BOOST_TRANSACT_DETAIL_FILEBUF_FILE_HPP

#include <fstream>
#include <string>
#include <boost/mpl/size_t.hpp>
#include <boost/filesystem.hpp>
#include <boost/transact/exception.hpp>
#include <boost/transact/detail/file.hpp>

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4996)
#endif

namespace boost {
  namespace transact {
    namespace detail {

      class filebuf_seq_ofile {
      public:

        typedef unsigned int size_type;

        static bool const has_direct_io = false;

        explicit filebuf_seq_ofile(std::string const &name) :
            pos(0), name(name) {
          this->open();
        }

        void write(void const *data, mpl::size_t<1>) {
          if (this->buf.sputc(*static_cast<char const *>(data)) == EOF) throw io_failure();
          ++this->pos;
        }

        void write(void const *data, std::size_t size) {
          std::streamsize ret = this->buf.sputn(static_cast<char const *>(data), std::streamsize(size));
          this->pos += ret;
          if (ret != std::streamsize(size)) throw io_failure();
        }

        size_type position() const { return this->pos; }

        void flush() { if (this->buf.pubsync() != 0) throw io_failure(); }

        void sync() { throw unsupported_operation(); }

        void close() { if (!this->buf.close()) throw io_failure(); }

        void reopen(const std::string& newname) {
          BOOST_ASSERT(!this->buf.is_open());
          filesystem::remove(this->name);
          this->name = newname;
          this->open();
        }

      private:

        void open() {
          if (!this->buf.open(this->name.c_str(), std::ios::out | std::ios::binary)) throw io_failure();
          this->pos = 0;
        }

        std::filebuf buf;
        size_type pos;
        std::string name;
      };

      class filebuf_seq_ifile {
      public:

        typedef unsigned long long size_type;

        explicit filebuf_seq_ifile(std::string const &name) :
            pos(0) {
          if (!this->buf.open(name.c_str(), std::ios::in | std::ios::binary)) throw io_failure();
        }

        void read(void *dataptr, mpl::size_t<1>) {
          char &data = *static_cast<char *>(dataptr);
          int ret = this->buf.sbumpc();
          if (ret == EOF) throw eof_exception();
          ++this->pos;
          data = ret;
        }

        void read(void *data, std::size_t size) {
          std::streamsize ret = this->buf.sgetn(static_cast<char *>(data), std::streamsize(size));
          this->pos += ret;
          if (ret != std::streamsize(size)) {
            if (ret == 0) throw eof_exception();
            else throw io_failure();
          }
        }

        size_type position() const {
          return this->pos;
        }

      private:

        std::filebuf buf;
        size_type pos;
      };

    }
  }
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

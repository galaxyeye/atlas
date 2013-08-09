//               Copyright Stefan Strasser 2010 - 2013
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TRANSACT_DETAIL_FILE_HPP
#define BOOST_TRANSACT_DETAIL_FILE_HPP

#include <boost/filesystem.hpp>
#include <boost/transact/exception.hpp>

namespace boost {
  namespace transact {

    struct eof_exception : io_failure {
    };

    namespace detail {

#ifdef _WIN32
#include <Windows.h>
#include <WinBase.h>

#include <strsafe.h>

      static void throw_io_failure(char const* function)
      {
        // Retrieve the system error message for the last-error code
        DWORD dw = GetLastError();

        LPVOID lpMsgBuf;
        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

        // Display the error message and exit the process
        // TODO - should be incorporated into io_failure what().
        std::cerr << function << " failed with error " << dw << ": " << (char*)lpMsgBuf << std::endl;

        LocalFree(lpMsgBuf);
        throw io_failure();
      }

// low-level ofile representation for WIN32
      template <bool direct_io = false>
      class ofile {
      public:
        typedef unsigned int size_type;
        static const bool has_direct_io = direct_io;

        void* filedes;

        ofile(std::string const &name) : filedes(INVALID_HANDLE_VALUE) {
          unsigned long access = GENERIC_READ | GENERIC_WRITE;
          unsigned long creation_flags = OPEN_ALWAYS;
          unsigned long flags = 0;
          if ( direct_io )
          flags |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;

          this->filedes = CreateFileA(name.c_str(), access,
              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
              0, creation_flags, flags, 0);
          if (this->filedes == INVALID_HANDLE_VALUE )
          throw_io_failure("CreateFileA");

          //make sure the directory entry has reached the disk:
          std::string dirname=filesystem::system_complete(name).parent_path().native();

          creation_flags = OPEN_EXISTING;
          flags = FILE_FLAG_BACKUP_SEMANTICS;
          void *dirfiledes = CreateFileA(dirname.c_str(), access,
              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
              0, creation_flags, flags, 0);
          if (dirfiledes == INVALID_HANDLE_VALUE )
          throw_io_failure("CreateFileA");
          if(!FlushFileBuffers(dirfiledes))
          throw_io_failure("FlushFileBuffers");
          if(!CloseHandle(dirfiledes))
          throw_io_failure("CloseHandle");
        }

        ~ofile() {
          if(this->filedes != INVALID_HANDLE_VALUE)
          CloseHandle(this->filedes);
        }

        void close(); //TODO
        void reopen(std::string const &name);//TODO

        void seek(size_type const &s) {
          LARGE_INTEGER loc;
          loc.QuadPart = s;
          if(SetFilePointerEx(this->filedes, loc, NULL, FILE_BEGIN) == 0)
          throw_io_failure("SetFilePointerEx");
        }

        size_type write(const char *data, size_type const &size) {
          DWORD written;
          if(WriteFile(this->filedes, data, size, &written, 0) == 0)
          throw_io_failure("WriteFile");
          return (size_type)written;
        }
        void flush() {}
        void sync() {
          if (!direct_io && FlushFileBuffers(this->filedes) == 0)
          throw_io_failure("FlushFileBuffers");
        }
      };

#else

#include <unistd.h>
#include <fcntl.h>

#ifndef _POSIX_SYNCHRONIZED_IO
#error no POSIX synchronized IO available
#endif

// low-level ofile for Linux/Unix
      template<bool direct_io = false> // ignored on Posix API.
      class ofile {
      public:

        typedef unsigned int size_type;
        static const bool has_direct_io = direct_io;

        ofile(std::string const &name) :
            filedes(-1), name(name) {
          this->open_();
          this->sync_directory();
        }

        ~ofile() {
          this->close();
        }

        void seek(size_type const &s) {
          if (::lseek(this->filedes, s, SEEK_SET) != off_t(s)) throw io_failure();
        }

        size_type write(const char *data, size_type const &size) {
          if (::write(this->filedes, data, size) != ssize_t(size)) throw io_failure();
          return size;
        }

        void flush() {
        }

        void sync() {
#ifdef linux
          if (::fdatasync(this->filedes) != 0) throw io_failure();
#else //multiple sources say fdatasync is not safe on other systems
          if(::fsync(this->filedes) != 0) throw io_failure();
#endif		
        }

        void close() {
          if (this->filedes != -1) {
            if (::close(this->filedes) != 0) throw io_failure();
          }
          this->filedes = -1;
        }

        void reopen(std::string const &newname) {
          BOOST_ASSERT(this->filedes == -1);
          filesystem::remove(this->name);
          this->name = newname;
          this->open_();
          sync_directory();
        }

        void rename(std::string const &newname) {
          BOOST_ASSERT(this->filedes == -1);
          filesystem::rename(this->name, newname);
          this->name = newname;
          this->open_(0); //do not truncate file!
          sync_directory();
        }

      private:

        void sync_directory() {
          //TODO optimization: directory entries are synced even if syncing is turning off,
          //which is not passed to ofile
          std::string dirname = filesystem::path(this->name).parent_path().native();
          if (dirname.empty()) dirname = ".";
          int dirfd = open(dirname.c_str(), O_RDONLY);
          if (dirfd == -1) throw io_failure();
          int ret = ::fsync(dirfd);
          if (::close(dirfd) != 0 || ret != 0) throw io_failure();
        }

        void open_(int flags = O_CREAT | O_TRUNC) {
          flags |= O_WRONLY;
#ifdef linux
          flags |= O_NOATIME;
#endif
          this->filedes = open(this->name.c_str(), flags, S_IRUSR | S_IWUSR);
          if (this->filedes == -1) throw io_failure();
        }

        int filedes;
        std::string name;
      };

#endif

      typedef ofile<true> direct_ofile;

    }
  }
}

#endif

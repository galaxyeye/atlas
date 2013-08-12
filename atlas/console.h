/*
 * console.h
 *
 *  Created on: Aug 9, 2013
 *      Author: vincent
 */

#ifndef ATLAS_CONSOLE_H_
#define ATLAS_CONSOLE_H_

#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>

namespace atlas {

  static const int COMMANDLINE_DEFAULT_HISTORY_MAX_LEN = 100;
  static const int COMMANDLINE_MAX_LINE = 100;
  static const char* DEFAULT_HISTORY_PATH = "/tmp/atlas_console_history";
  static const char* UNSUPPORTED_TERM[] = { "dumb", "cons25", nullptr };
  static int HISTORY_MAX_LEN = COMMANDLINE_DEFAULT_HISTORY_MAX_LEN;

  struct commandline_completions {
    size_t len;
    char **cvec;
  };

  class console {
  private:

    termios _orig_termios;
    int _rawmode; /* for at_exit() function to check if restore is needed*/
    int _history_len;
    int _history_index;

    char** _history;
    typedef void(completion_callback)(const char *buf, commandline_completions *lc);
    completion_callback* completionCallback;

    std::string _prompt;
    std::string _history_path;

  public:

    console(const char* prompt, const char* history_path = DEFAULT_HISTORY_PATH)
      : _rawmode(0),
        _history_len(0),
        _history_index(0),
        _history(nullptr),
        completionCallback(nullptr),
        _prompt(prompt),
        _history_path(history_path)
    {
      load_history(_history_path.c_str());
    }

    ~console() {
      disable_raw_mode(STDIN_FILENO);
      save_history(_history_path.c_str());

      free_history();
    }

  public:

    const char* getline() {
      char buf[COMMANDLINE_MAX_LINE];
      int count;

      if (is_unsupported_term()) {
        size_t len;

        printf("%s", _prompt.c_str());
        fflush(stdout);
        if (fgets(buf, COMMANDLINE_MAX_LINE, stdin) == nullptr) return nullptr;
        len = strlen(buf);
        while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
          len--;
          buf[len] = '\0';
        }

        return strdup(buf);
      }
      else {
        count = command_line_raw(buf, COMMANDLINE_MAX_LINE, _prompt.c_str());
        if (count == -1) return nullptr;
        return strdup(buf);
      }
    }

    /* Register a callback function to be called for tab-completion. */
    void set_completion_callback(void(*fn)(char const*, commandline_completions*)) {
      completionCallback = fn;
    }

    /* Using a circular buffer is smarter, but a bit more complex to handle. */
    int add_history(const char *line);

  protected:

    /* Load the history from the specified file. If the file does not exist
     * zero is returned and no operation is performed.
     *
     * If the file exists and the operation succeeded 0 is returned, otherwise
     * on error -1 is returned. */
    int load_history(const char *filename);

    /* Save the history in the specified file. On success 0 is returned
     * otherwise -1 is returned. */
    int save_history(const char *filename);

    void free_history();

  protected:

    int is_unsupported_term() {
      char *term = getenv("TERM");
      int j;

      if (term == nullptr) return 0;
      for (j = 0; UNSUPPORTED_TERM[j]; j++)
        if (!strcasecmp(term, UNSUPPORTED_TERM[j])) return 1;
      return 0;
    }

    int enable_raw_mode(int fd);

    void disable_raw_mode(int fd) {
      /* Don't even check the return value as it's too late. */
      if (_rawmode && tcsetattr(fd, TCSADRAIN, &_orig_termios) != -1) _rawmode = 0;
    }

    int get_columns() {
      struct winsize ws;

      if (ioctl(1, TIOCGWINSZ, &ws) == -1) return 80;
      return ws.ws_col;
    }

    void refresh_line(int fd, const char *prompt, char *buf, size_t len, size_t pos,
        size_t cols);

    /* Note that this should parse some special keys into their emacs ctrl-key combos
     * Return of -1 signifies unrecognized code
     */
    char read_char(int fd);

    void beep() {
      fprintf(stderr, "\x7");
      fflush(stderr);
    }

    void free_completions(commandline_completions *lc) {
      size_t i;
      for (i = 0; i < lc->len; i++)
        free(lc->cvec[i]);
      if (lc->cvec != nullptr) free(lc->cvec);
    }

    int complete_line(int fd, const char *prompt, char *buf, size_t buflen, size_t *len,
        size_t *pos, size_t cols);

    void clear_screen() {
      if (write(1, "\x1b[H\x1b[2J", 7) <= 0) {
        /* nothing to do, just to avoid warning. */
      }
    }

    int prompt_cmd_line(int fd, char *buf, size_t buflen, const char* prompt);

    int command_line_raw(char *buf, size_t buflen, const char* prompt);

    void add_completion(commandline_completions* lc, const char* str) {
      size_t len = strlen(str);
      char *copy = (char*) malloc(len + 1);
      memcpy(copy, str, len + 1);
      lc->cvec = (char**) realloc(lc->cvec, sizeof(char*) * (lc->len + 1));
      lc->cvec[lc->len++] = copy;
    }

    int set_history_max_len(int len) {
      char** newHistory;

      if (len < 1) return 0;

      if (_history) {
        int tocopy = _history_len;

        newHistory = (char**) malloc(sizeof(char*) * len);
        if (newHistory == nullptr) return 0;
        if (len < tocopy) tocopy = len;
        memcpy(newHistory, _history + (HISTORY_MAX_LEN - tocopy), sizeof(char*) * tocopy);
        free(_history);
        _history = newHistory;
      }

      HISTORY_MAX_LEN = len;
      if (_history_len > HISTORY_MAX_LEN) _history_len = HISTORY_MAX_LEN;
      return 1;
    }
  };

  /* Load the history from the specified file. If the file does not exist
   * zero is returned and no operation is performed.
   *
   * If the file exists and the operation succeeded 0 is returned, otherwise
   * on error -1 is returned. */
  int console::load_history(const char *filename) {
    FILE *fp = fopen(filename, "r");
    char buf[COMMANDLINE_MAX_LINE];

    if (fp == nullptr) return -1;

    while (fgets(buf, COMMANDLINE_MAX_LINE, fp) != nullptr) {
      char *p;

      p = strchr(buf, '\r');
      if (!p) p = strchr(buf, '\n');
      if (p) *p = '\0';
      if (p != buf) add_history(buf);
    }
    fclose(fp);
    return 0;
  }

  /* Save the history in the specified file. On success 0 is returned
   * otherwise -1 is returned. */
  int console::save_history(const char *filename) {
    FILE *fp = fopen(filename, "w");
    int j;

    if (fp == nullptr) return -1;
    for (j = 0; j < _history_len; j++) {
      if (_history[j][0] != '\0') fprintf(fp, "%s\n", _history[j]);
    }
    fclose(fp);
    return 0;
  }


  /* Using a circular buffer is smarter, but a bit more complex to handle. */
  int console::add_history(const char *line) {
    char *linecopy;

    if (HISTORY_MAX_LEN == 0) return 0;
    if (_history == nullptr) {
      _history = (char**) malloc(sizeof(char*) * HISTORY_MAX_LEN);
      if (_history == nullptr) return 0;
      memset(_history, 0, (sizeof(char*) * HISTORY_MAX_LEN));
    }

    linecopy = strdup(line);
    if (!linecopy) return 0;

    if (_history_len == HISTORY_MAX_LEN) {
      free(_history[0]);
      memmove(_history, _history + 1, sizeof(char*) * (HISTORY_MAX_LEN - 1));
      _history_len--;
    }

    _history[_history_len] = linecopy;
    _history_len++;

    return 1;
  }

  void console::free_history() {
    if (_history) {
      int j;

      for (j = 0; j < _history_len; j++)
        free(_history[j]);
      free(_history);
    }
  }

  int console::enable_raw_mode(int fd) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto fatal;

    if (tcgetattr(fd, &_orig_termios) == -1) goto fatal;

    raw = _orig_termios; /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    // raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd, TCSADRAIN, &raw) < 0) goto fatal;
    _rawmode = 1;
    return 0;

    fatal:
    errno = ENOTTY;
    return -1;
  }

  char console::read_char(int fd) {
    char c;
    int nread;
    char seq[2], seq2[2];

    nread = read(fd, &c, 1);
    if (nread <= 0) return 0;

    if (c == 27) { /* escape */
      if (read(fd, seq, 2) == -1) return 0;
      if (seq[0] == 91) {
        if (seq[1] == 68) { /* left arrow */
          return 2; /* ctrl-b */
        }
        else if (seq[1] == 67) { /* right arrow */
          return 6; /* ctrl-f */
        }
        else if (seq[1] == 65) { /* up arrow */
          return 16; /* ctrl-p */
        }
        else if (seq[1] == 66) { /* down arrow */
          return 14; /* ctrl-n */
        }
        else if (seq[1] > 48 && seq[1] < 57) {
          /* extended escape */
          if (read(fd, seq2, 2) == -1) return 0;
          if (seq2[0] == 126) {
            if (seq[1] == 49 || seq[1] == 55) { /* home (linux console and rxvt based) */
              return 1; /* ctrl-a */
            }
            else if (seq[1] == 52 || seq[1] == 56) { /* end (linux console and rxvt based) */
              return 5; /* ctrl-e */
            }
            else if (seq[1] == 51) { /* delete */
              return 127; /* ascii DEL byte */
            }
            else {
              return -1;
            }
          }
          else {
            return -1;
          }
          if (seq[1] == 51 && seq2[0] == 126) { /* delete */
            return 127; /* ascii DEL byte */
          }
          else {
            return -1;
          }
        }
        else {
          return -1;
        }
      }
      else if (seq[0] == 79) {
        if (seq[1] == 72) { /* home (xterm based) */
          return 1; /* ctrl-a */
        }
        else if (seq[1] == 70) { /* end (xterm based) */
          return 5; /* ctrl-e */
        }
        else {
          return -1;
        }
      }
      else {
        return -1;
      }
    }
    else if (c == 127) {
      /* some consoles use 127 for backspace rather than delete.
       * we only use it for delete */
      return 8;
    }

    return c; /* normalish character */
  }

  void console::refresh_line(int fd, const char *prompt, char *buf, size_t len, size_t pos,
      size_t cols) {
    size_t plen = strlen(prompt);

    while ((plen + pos) >= cols) {
      buf++;
      len--;
      pos--;
    }
    while (plen + len > cols) {
      len--;
    }

    {
      char seq[64];
      int highlight = -1;

      if (pos < len) {
        /* this scans for a brace matching buf[pos] to highlight */
        int scanDirection = 0;
        if (strchr("}])", buf[pos])) scanDirection = -1; /* backwards */
        else if (strchr("{[(", buf[pos])) scanDirection = 1; /* forwards */

        if (scanDirection) {
          int unmatched = scanDirection;
          int i;
          for (i = pos + scanDirection; i >= 0 && i < (int) len; i += scanDirection) {
            /* TODO: the right thing when inside a string */
            if (strchr("}])", buf[i])) unmatched--;
            else if (strchr("{[(", buf[i])) unmatched++;

            if (unmatched == 0) {
              highlight = i;
              break;
            }
          }
        }
      }

      /* Cursor to left edge */
      snprintf(seq, 64, "\x1b[1G");
      if (write(fd, seq, strlen(seq)) == -1) return;
      /* Write the prompt and the current buffer content */
      if (write(fd, prompt, strlen(prompt)) == -1) return;

      if (highlight == -1) {
        if (write(fd, buf, len) == -1) return;
      }
      else {
        if (write(fd, buf, highlight) == -1) return;
        if (write(fd, "\x1b[1;34m", 7) == -1) return; /* bright blue (visible with both B&W bg) */
        if (write(fd, &buf[highlight], 1) == -1) return;
        if (write(fd, "\x1b[0m", 4) == -1) return; /* reset */
        if (write(fd, buf + highlight + 1, len - highlight - 1) == -1) return;
      }

      /* Erase to right */
      snprintf(seq, 64, "\x1b[0K");
      if (write(fd, seq, strlen(seq)) == -1) return;
      /* Move cursor to original position. */
      snprintf(seq, 64, "\x1b[1G\x1b[%dC", (int) (pos + plen));
      if (write(fd, seq, strlen(seq)) == -1) return;
    }
  }

  int console::complete_line(int fd, const char *prompt, char *buf, size_t buflen, size_t *len,
      size_t *pos, size_t cols) {
    commandline_completions lc = { 0, nullptr };
    int nwritten;
    char c = 0;

    completionCallback(buf, &lc);
    if (lc.len == 0) {
      beep();
    }
    else {
      size_t stop = 0, i = 0;
      size_t clen;

      while (!stop) {
        /* Show completion or original buffer */
        if (i < lc.len) {
          clen = strlen(lc.cvec[i]);
          refresh_line(fd, prompt, lc.cvec[i], clen, clen, cols);
        }
        else {
          refresh_line(fd, prompt, buf, *len, *pos, cols);
        }

        do {
          c = read_char(fd);
        } while (c == (char) -1);

        switch (c) {
        case 0:
          free_completions(&lc);
          return -1;
        case 9: /* tab */
          i = (i + 1) % (lc.len + 1);
          if (i == lc.len) beep();
          break;
        case 27: /* escape */
          /* Re-show original buffer */
          if (i < lc.len) {
            refresh_line(fd, prompt, buf, *len, *pos, cols);
          }
          stop = 1;
          break;
        default:
          /* Update buffer and return */
          if (i < lc.len) {
            nwritten = snprintf(buf, buflen, "%s", lc.cvec[i]);
            *len = *pos = nwritten;
          }
          stop = 1;
          break;
        }
      }
    }

    free_completions(&lc);
    return c; /* Return last read character */
  }

  int console::prompt_cmd_line(int fd, char *buf, size_t buflen, const char *prompt) {
    size_t plen = strlen(prompt);
    size_t pos = 0;
    size_t len = 0;
    size_t cols = get_columns();

    buf[0] = '\0';
    buflen--; /* Make sure there is always space for the nulterm */

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    add_history("");
    _history_index = _history_len - 1;

    if (write(1, prompt, plen) == -1) return -1;
    while (1) {
      char c = read_char(fd);

      if (c == 0) return len;
      if (c == (char) -1) {
        refresh_line(fd, prompt, buf, len, pos, cols);
        continue;
      }

      /* Only autocomplete when the callback is set. It returns < 0 when
       * there was an error reading from fd. Otherwise it will return the
       * character that should be handled next. */
      if (c == 9 && completionCallback != nullptr) { /* tab */
        /* ignore tabs used for indentation */
        if (pos == 0) continue;

        c = complete_line(fd, prompt, buf, buflen, &len, &pos, cols);
        /* Return on errors */
        if (c < 0) return len;
        /* Read next character when 0 */
        if (c == 0) continue;
      }

      switch (c) {
      case 13: /* enter */
        _history_len--;
        free(_history[_history_len]);
        return (int) len;
      case 3: /* ctrl-c */
        // errno = EAGAIN;
        //  return -1;
        break;
      case 127: /* delete */
        if (len > 0 && pos < len) {
          memmove(buf + pos, buf + pos + 1, len - pos - 1);
          len--;
          buf[len] = '\0';
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        break;
      case 8: /* backspace or ctrl-h */
        if (pos > 0 && len > 0) {
          memmove(buf + pos - 1, buf + pos, len - pos);
          pos--;
          len--;
          buf[len] = '\0';
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        break;
      case 4: /* ctrl-d, remove char at right of cursor */
        if (len > 1 && pos < (len - 1)) {
          memmove(buf + pos, buf + pos + 1, len - pos);
          len--;
          buf[len] = '\0';
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        else if (len == 0) {
          _history_len--;
          free(_history[_history_len]);
          return -1;
        }
        break;
      case 20: /* ctrl-t */
        if (pos > 0 && pos < len) {
          int aux = buf[pos - 1];
          buf[pos - 1] = buf[pos];
          buf[pos] = aux;
          if (pos != len - 1) pos++;
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        break;
      case 2: /* ctrl-b *//* left arrow */
        if (pos > 0) {
          pos--;
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        break;
      case 6: /* ctrl-f */
        /* right arrow */
        if (pos != len) {
          pos++;
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        break;
      case 16: /* ctrl-p */
      case 14: /* ctrl-n */
        /* up and down arrow: history */
        if (_history_len > 1) {
          /* Update the current history entry before to
           * overwrite it with tne next one. */
          free(_history[_history_index]);
          _history[_history_index] = strdup(buf);
          /* Show the new entry */
          _history_index += (c == 16) ? -1 : 1;
          if (_history_index < 0) {
            _history_index = 0;
            break;
          }
          else if (_history_index >= _history_len) {
            _history_index = _history_len - 1;
            break;
          }
          strncpy(buf, _history[_history_index], buflen);
          buf[buflen] = '\0';
          len = pos = strlen(buf);
          refresh_line(fd, prompt, buf, len, pos, cols);
        }
        break;
      case 27: /* escape sequence */
        break; /* should be handled by read_char */
      default:
        if (len < buflen) {
          if (len == pos) {
            buf[pos] = c;
            pos++;
            len++;
            buf[len] = '\0';
            if (plen + len < cols) {
              /* Avoid a full update of the line in the
               * trivial case. */
              if (write(1, &c, 1) == -1) return -1;
            }
            else {
              refresh_line(fd, prompt, buf, len, pos, cols);
            }
          }
          else {
            memmove(buf + pos + 1, buf + pos, len - pos);
            buf[pos] = c;
            len++;
            pos++;
            buf[len] = '\0';
            refresh_line(fd, prompt, buf, len, pos, cols);
          }
        }
        break;
      case 21: /* Ctrl+u, delete the whole line. */
        buf[0] = '\0';
        pos = len = 0;
        refresh_line(fd, prompt, buf, len, pos, cols);
        break;
      case 11: /* Ctrl+k, delete from current to end of line. */
        buf[pos] = '\0';
        len = pos;
        refresh_line(fd, prompt, buf, len, pos, cols);
        break;
      case 1: /* Ctrl+a, go to the start of the line */
        pos = 0;
        refresh_line(fd, prompt, buf, len, pos, cols);
        break;
      case 5: /* ctrl+e, go to the end of the line */
        pos = len;
        refresh_line(fd, prompt, buf, len, pos, cols);
        break;
      case 12: /* ctrl+l, clear screen */
        clear_screen();
        refresh_line(fd, prompt, buf, len, pos, cols);
        break;
      }
    }

    return len;
  }

  int console::command_line_raw(char *buf, size_t buflen, const char *prompt) {
    int fd = STDIN_FILENO;
    int count;

    if (buflen == 0) {
      errno = EINVAL;
      return -1;
    }

    if (!isatty(STDIN_FILENO)) {
      if (fgets(buf, buflen, stdin) == nullptr) return -1;
      count = strlen(buf);
      if (count && buf[count - 1] == '\n') {
        count--;
        buf[count] = '\0';
      }
    }
    else {
      if (enable_raw_mode(fd) == -1) return -1;
      count = prompt_cmd_line(fd, buf, buflen, prompt);
      disable_raw_mode(fd);
      printf("\n\r");
    }

    return count;
  }
} // atlas

#endif /* CONSOLE_H_ */

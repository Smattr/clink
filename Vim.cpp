#include <array>
#include <cassert>
#include "colours.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <string>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include "Vim.h"
#include <vector>

using namespace std;

static int run(const char **argv, bool mask_stdout = false) {

  int p[2];
  if (pipe(p) < 0)
    return EXIT_FAILURE;

  if (fcntl(p[1], F_SETFD, FD_CLOEXEC) < 0) {
    close(p[0]);
    close(p[1]);
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(p[0]);
    close(p[1]);
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    close(p[0]);

    if (mask_stdout) {
      /* Here we need to create a new PTY, rather than simply duping
       * /dev/null over the top of std*. If Vim detects stdout is not a
       * TTY, it throws a warning and its processing somehow slows down. I
       * haven't investigated, but I suspect this is actually an artefact
       * of OS interaction, rather than anything specific Vim is doing.
       */
      int fd = posix_openpt(O_RDWR);
      if (fd < 0)
        goto fail;
      if (dup2(fd, STDIN_FILENO) < 0 ||
          dup2(fd, STDOUT_FILENO) < 0 ||
          dup2(fd, STDERR_FILENO) < 0)
        goto fail;
    }

    (void)execvp(argv[0], const_cast<char *const*>(argv));

fail:
    char c = 0;
    (void)write(p[1], &c, sizeof(c));

    exit(EXIT_FAILURE);
  }

  close(p[1]);

  char ignored;
  ssize_t r;
  do {
    r = read(p[0], &ignored, sizeof(ignored));
  } while (r == -1 && errno == EINTR);
  if (r == 1) {
    close(p[0]);
    return EXIT_FAILURE;
  }

  close(p[0]);

  int status;
  if (waitpid(pid, &status, 0) < 0)
    return EXIT_FAILURE;

  if (WIFEXITED(status))
    return WEXITSTATUS(status);

  return EXIT_FAILURE;
}

int vim_open(const string &filename, unsigned line, unsigned col) {
  string cursor = "+call cursor(" + to_string(line) + "," + to_string(col) + ")";
  char const *argv[] = { "vim", cursor.c_str(), filename.c_str(), nullptr };
  return run(argv);
}

#ifdef TEST_VIM_OPEN
static void usage(const string &prog) {
  cerr << "usage: " << prog << " filename line col\n";
}

int main(int argc, char **argv) {
  if (argc != 4) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  const string filename = argv[1];
  unsigned line, col;

  try {
    line = stoul(argv[2]);
    col = stoul(argv[3]);
  } catch (exception &e) {
    cerr << e.what() << "\n";
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  return vim_open(filename, line, col);
}
#endif

/******************************************************************************
 *                                                                            *
 * The remainder of this file is essentially insanity. Don't say I didn't     *
 * warn you.                                                                  *
 *                                                                            *
 * ----                                                                       *
 *                                                                            *
 * We would like to display the contents of a file in a terminal, with syntax *
 * highlighting as if it were in Vim. This isn't as straightforward as it     *
 * sounds because a user's settings are not known (e.g. they might have       *
 * extended a language with further keywords). There are two obvious ideas    *
 * for how to achieve this:                                                   *
 *                                                                            *
 *   1. Teach Clink how to parse ~/.vimrc; or                                 *
 *   2. Call out to Vimcat.                                                   *
 *                                                                            *
 * Parsing ~/.vimrc is almost an immediate non-starter. The Vim configuration *
 * syntax is complex, highly dynamic and not well documented. Parsing a       *
 * user's Vim settings correctly is likely to involve more complexity than    *
 * the rest of Clink combined.                                                *
 *                                                                            *
 * Using Vimcat is attractive, but despite my best efforts I haven't been     *
 * able to get any of the variants of Vimcat floating around on the internet  *
 * to work reliably.                                                          *
 *                                                                            *
 * So what is one to do? Well, brood on the problem for a while before        *
 * landing on an idea so crazy it just might work. Vim ships with a tool      *
 * called 2html.vim that generates an HTML document with syntax highlighting  *
 * of the file you have open. So all we need to do is puppet Vim into running *
 * :TOhtml, parse the generated HTML and render the result using ANSI color   *
 * codes. It actually doesn't sound so bad when you say it in a nice short    *
 * sentence like that.
 *                                                                            *
 *****************************************************************************/

static int convert_to_html(const string &input, const string &output) {
  /* Construct a command line to open the file in Vim, convert it to highlighted
   * HTML, save this to the output and exit. 
   * FIXME wrap it in timeout
   * in case the user has a weird ~/.vimrc and we end up hanging.
   */
  string save = "+w " + output;
  char const *command[] = { "vim", "-n", "+set nonumber", "+TOhtml",
      save.c_str(), "+qa!", input.c_str(), nullptr };

  return run(command, true);
}

namespace {

struct Style {
  unsigned fg;
  unsigned bg;
  bool bold;
  bool underline;
};

}

/* Decode a fragment of HTML text produced by Vim's TOhtml. It is assumed that
 * the input contains no HTML tags.
 */
static string from_html(const unordered_map<string, Style> &styles,
    const string &text) {

  // The HTML escapes produced by TOhtml.
  struct translation_t {
    const char *key;
    char value;
  };
  static const array<translation_t, 5> HTML_DECODE { {
    { "amp;", '&' },
    { "gt;", '>' },
    { "lt;", '<' },
    { "nbsp;", ' ' },
    { "quot;", '"' },
  } };

  string result;
  for (size_t i = 0; i < text.size(); i++) {

    static const string SPAN_OPEN = "<span class=\"";
    static const string SPAN_CLOSE = "</span>";

    if (text[i] == '&' && i + 1 < text.size()) {
      // An HTML escape; let's see if we can decode it.
      bool translated = false;
      for (const translation_t &t : HTML_DECODE) {
        if (text.compare(i + 1, strlen(t.key), t.key) == 0) {
          result += t.value;
          i += strlen(t.key);
          translated = true;
          break;
        }
      }

      if (translated)
        continue;
    }

    else if (text[i] == '<') {

      // Is this a <span class="..."> ?
      if (text.compare(i, SPAN_OPEN.size(), SPAN_OPEN) == 0) {
        size_t start = i + SPAN_OPEN.size();
        size_t end = text.find("\">", start);
        if (end != string::npos) {
          string style_name = text.substr(start, end - start);
          auto it = styles.find(style_name);
          if (it != styles.end()) {
            Style s = it->second;
            result += "\033[3" + to_string(s.fg) + ";4" + to_string(s.bg);
            if (s.bold)
              result += ";1";
            if (s.underline)
              result += ";4";
            result += "m";
            i += end - i + 1;
            continue;
          }
        }
      }
      
      // How about a </span> ?
      else if (text.compare(i, SPAN_CLOSE.size(), SPAN_CLOSE) == 0) {
        result += "\033[0m";
        i += SPAN_CLOSE.size() - 1;
        continue;
      }
    }

    result += text[i];
  }

  return result;
}

namespace {

class TemporaryDirectory {

 public:

  TemporaryDirectory() {

    const char *TMPDIR = getenv("TMPDIR");
    if (TMPDIR == nullptr) {
      TMPDIR = "/tmp";
    }

    char *temp;
    if (asprintf(&temp, "%s/tmp.XXXXXX", TMPDIR) < 0)
      return;

    if (mkdtemp(temp) == nullptr) {
      free(temp);
      return;
    }

    dir = temp;
    free(temp);
  }

  const string &get_path() const {
    return dir;
  }

  ~TemporaryDirectory() {
    if (dir != "")
      (void)rmdir(dir.c_str());
  }

 private:
  string dir;

};

}

vector<string> vim_highlight(const string &filename) {

  vector<string> lines;

  // Create a temporary directory to use for scratch space.
  TemporaryDirectory temp;
  if (temp.get_path() == "")
    return lines;

  // Convert the input to highlighted HTML.
  string output = temp.get_path() + "/temp.html";
  if (convert_to_html(filename, output) != 0)
    return lines;

  FILE *html = fopen(output.c_str(), "r");
  if (html == nullptr)
    return lines;

  /* The file we have open at this point has a structure like the following:
   *
   *     <html>
   *     ...
   *     <style type="text/css">
   *     ...
   *     .MyStyle1 { color: #ff00ff ...
   *     ...
   *     </style>
   *     ...
   *     <pre id='vimCodeElement'>
   *     Hello <span class="MyStyle1">world</span>!
   *     ...
   *     </pre>
   *     ...
   *
   * Because we know its structure, we're going to discard all the advice your
   * mother told you and parse HTML and CSS using a combination of regexing and
   * string searching. Hey, I warned you this wasn't going to be pleasant.
   */

  regex_t style;
  if (regcomp(&style,
          "^\\.([[:alpha:]][[:alnum:]]+)[[:blank:]]*\\{[[:blank:]]*"
          "(color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*)?"
          "(background-color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*"
           "(padding-bottom:[[:blank:]]*1px;[[:blank:]]*)?)?"
          "(font-weight:[[:blank:]]*bold;[[:blank:]]*)?"
          "(font-style:[[:blank:]]*italic;[[:blank:]]*)?"
          "(text-decoration:[[:blank:]]*underline;[[:blank:]]*)?",
          REG_EXTENDED) != 0) {
    fclose(html);
    (void)remove(output.c_str());
    return lines;
  }

  char *last_line = nullptr;
  size_t last_line_sz = 0;
  unordered_map<string, Style> styles;

  for (;;) {

    if (getline(&last_line, &last_line_sz, html) == -1) {
      regfree(&style);
      fclose(html);
      (void)remove(output.c_str());
      return lines;
    }

    if (last_line[0] == '.') {

      /* Setup for extraction of CSS attributes. The entries of match will be:
       *
       *   match[0]: the entire expression                             (ignore)
       *   match[1]: the name of the style
       *   match[2]: "color: #......;"                                 (ignore)
       *   match[3]:          ^^^^^^
       *   match[4]: "background-color: #......; padding-bottom: 1px;" (ignore)
       *   match[5]:                     ^^^^^^
       *   match[6]:                             ^^^^^^^^^^^^^^^^^^^^  (ignore)
       *   match[7]: "font-weight: bold;"
       *   match[8]: "font-style: italic;"                             (ignore for now)
       *   match[9]: "text-decoration: underline;"
       */
      regmatch_t match[10];
      static const size_t NMATCH = sizeof(match) / sizeof(match[0]);

      // Extract CSS attributes from this line.
      if (regexec(&style, last_line, NMATCH, match, 0) == REG_NOMATCH)
        continue;

      // Build a style defining these attributes.
      Style s { .fg = 9, .bg = 9, .bold = false, .underline = false };
      if (match[2].rm_so != -1)
        s.fg = html_colour_to_ansi(&last_line[match[3].rm_so], 6);
      if (match[4].rm_so != -1)
        s.bg = html_colour_to_ansi(&last_line[match[5].rm_so], 6);
      if (match[7].rm_so != -1)
        s.bold = true;
      if (match[9].rm_so != -1)
        s.underline = true;

      // Register this style for later highlighting.
      string name(last_line, match[1].rm_so, match[1].rm_eo - match[1].rm_so);
      styles[name] = s;

    } else if (strcmp(last_line, "<pre id='vimCodeElement'>\n") == 0) {
      // We found the content itself. We're done.
      break;
    }

  }

  regfree(&style);

  for (;;) {

    if (getline(&last_line, &last_line_sz, html) == -1) {
      fclose(html);
      break;
    }

    if (strcmp(last_line, "</pre>\n") == 0) {
      fclose(html);
      break;
    }

    lines.push_back(from_html(styles, last_line));

  }

  free(last_line);
  (void)remove(output.c_str());

  return lines;
}

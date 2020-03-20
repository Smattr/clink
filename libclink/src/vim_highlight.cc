#include <cstddef>
#include <array>
#include <climits>
#include <clink/Error.h>
#include <clink/vim.h>
#include "colour.h"
#include <cstdio>
#include <iostream>
#include <functional>
#include <fstream>
#include <optional>
#include "Re.h"
#include "run.h"
#include <sstream>
#include <string>
#include "TemporaryDirectory.h"
#include <unordered_map>

// This file is essentially insanity. Do not say I did not warn you.
//
// ----
//
// We would like to display the contents of a file in a terminal, with syntax
// highlighting as if it were in Vim. This is not as straightforward as it
// sounds because a user’s settings are not known (e.g. they might have
// extended a language with further keywords). There are two obvious ideas
// for how to achieve this:
//
//   1. Teach Clink how to parse ~/.vimrc; or
//   2. Call out to Vimcat.
//
// Parsing ~/.vimrc is almost an immediate non-starter. The Vim configuration
// syntax is complex, highly dynamic and not well documented. Parsing a
// user’s Vim settings correctly is likely to involve more complexity than
// the rest of Clink combined.
//
// Using Vimcat is attractive, but despite my best efforts I have not been
// able to get any of the variants of Vimcat floating around on the internet
// to work reliably.
//
// So what is one to do? Well, brood on the problem for a while before
// landing on an idea so crazy it just might work. Vim ships with a tool
// called 2html.vim that generates an HTML document with syntax highlighting
// of the file you have open. So all we need to do is puppet Vim into running
// :TOhtml, parse the generated HTML and render the result using ANSI colour
// codes. It actually does not sound so bad when you say it in a nice short
// sentence like that.

namespace clink {

static int convert_to_html(const std::string &input, const std::string &output) {

  // construct a directive telling Vim to save to the given output path
  std::string save = "+w " + output;

  // Construct a command line to open the file in Vim, convert it to highlighted
  // HTML, save this to the output and exit. 
  // FIXME wrap it in timeout
  // in case the user has a weird ~/.vimrc and we end up hanging.
  char const *argv[] = { "vim", "-n", "+set nonumber", "+TOhtml", save.c_str(),
    "+qa!", input.c_str(), nullptr };

  return run(argv, true);
}

namespace {
struct Style {
  unsigned fg;
  unsigned bg;
  bool bold;
  bool underline;
};
}

// Decode a fragment of HTML text produced by Vim’s TOhtml. It is assumed that
// the input contains no HTML tags.
static std::string from_html(
    const std::unordered_map<std::string, Style> &styles,
    const std::string &line) {

  // the HTML escapes produced by TOhtml
  struct Translation {
    const char *key;
    char value;
  };
  static const Translation HTML_DECODE[] = {
    { "amp;", '&' },
    { "gt;", '>' },
    { "lt;", '<' },
    { "nbsp;", ' ' },
    { "quot;", '"' },
  };

  // setup a dynamic buffer to construct the output
  std::ostringstream result;

  for (size_t i = 0; i < line.size(); i++) {

    static const std::string SPAN_OPEN = "<span class=\"";
    static const std::string SPAN_CLOSE = "</span>";

    if (line[i] == '&' && i + 1 < line.size()) {
      // An HTML escape; let us see if we can decode it.
      bool translated = false;
      for (const Translation &t : HTML_DECODE) {
        if (line.compare(i + 1, strlen(t.key), t.key) == 0) {
          result << t.value;
          i += strlen(t.key);
          translated = true;
          break;
        }
      }

      if (translated)
        continue;
    }

    else if (line[i] == '<') {

      // is this a <span class="..."> ?
      if (line.compare(i, SPAN_OPEN.size(), SPAN_OPEN) == 0) {
        size_t start = i + SPAN_OPEN.size();
        size_t end = line.find("\">", start);
        if (end != std::string::npos) {
          std::string style_name = line.substr(start, end - start);
          auto it = styles.find(style_name);
          if (it != styles.end()) {
            const Style s = it->second;
            result << "\033[3" << s.fg << ";4" << s.bg;
            if (s.bold)
              result << ";1";
            if (s.underline)
              result << ";4";
            result << "m";
            i += end - i + 1;
            continue;
          }
        }
      }
      
      // how about a </span> ?
      else if (line.compare(i, SPAN_CLOSE.size(), SPAN_CLOSE) == 0) {
        result << "\033[0m";
        i += SPAN_CLOSE.size() - 1;
        continue;
      }
    }

    result << line[i];
  }

  return result.str();
}

int vim_highlight(const std::string &filename,
    std::function<int(const std::string&)> const &callback) {

  int rc = 0;

  // create a temporary directory to use for scratch space
  TemporaryDirectory temp;

  // convert the input to highlighted HTML
  std::string output = temp.get_path() + "/temp.html";
  if ((rc = convert_to_html(filename, output)))
    return rc;

  // open the generated HTML
  std::ifstream html(output);
  if (!html.is_open())
    throw Error("failed to open " + output);

  // The file we have open at this point has a structure like the following:
  //
  //     <html>
  //     ...
  //     <style type="text/css">
  //     ...
  //     .MyStyle1 { color: #ff00ff ...
  //     ...
  //     </style>
  //     ...
  //     <pre id='vimCodeElement'>
  //     Hello <span class="MyStyle1">world</span>!
  //     ...
  //     </pre>
  //     ...
  //
  // Because we know its structure, we are going to discard all the advice your
  // mother told you and parse HTML and CSS using a combination of regexing and
  // string searching. Hey, I warned you this was not going to be pleasant.

  static const char STYLE[] = "^\\.([[:alpha:]][[:alnum:]]+)[[:blank:]]*"
    "\\{[[:blank:]]*(color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*)?"
    "(background-color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*"
    "(padding-bottom:[[:blank:]]*1px;[[:blank:]]*)?)?"
    "(font-weight:[[:blank:]]*bold;[[:blank:]]*)?"
    "(font-style:[[:blank:]]*italic;[[:blank:]]*)?"
    "(text-decoration:[[:blank:]]*underline;[[:blank:]]*)?";
  Re<10> style(STYLE);

  std::unordered_map<std::string, Style> styles;

  for (;;) {

    std::string line;
    if (!std::getline(html, line))
      break;

    if (line[0] == '.') {

      // Setup for extraction of CSS attributes. The entries of match will be:
      //
      //   match[0]: the entire expression                             (ignore)
      //   match[1]: the name of the style
      //   match[2]: "color: #......;"                                 (ignore)
      //   match[3]:          ^^^^^^
      //   match[4]: "background-color: #......; padding-bottom: 1px;" (ignore)
      //   match[5]:                     ^^^^^^
      //   match[6]:                             ^^^^^^^^^^^^^^^^^^^^  (ignore)
      //   match[7]: "font-weight: bold;"
      //   match[8]: "font-style: italic;"                             (ignore for now)
      //   match[9]: "text-decoration: underline;"

      // is this a style definition line?
      std::optional<std::array<Match, 10>> m = style.matches(line);
      if (!m.has_value())
        continue;

      // build a style defining these attributes
      Style s = { .fg = 9, .bg = 9, .bold = false, .underline = false };
      if ((*m)[2].start_offset != SIZE_MAX)
        s.fg = html_colour_to_ansi(line.c_str() + (*m)[3].start_offset);
      if ((*m)[4].start_offset != SIZE_MAX)
        s.bg = html_colour_to_ansi(line.c_str() + (*m)[5].start_offset);
      if ((*m)[7].start_offset != SIZE_MAX)
        s.bold = true;
      if ((*m)[9].start_offset != SIZE_MAX)
        s.underline = true;

      // register this style for later highlighting
      size_t extent = (*m)[1].end_offset - (*m)[1].start_offset;
      std::string name = line.substr((*m)[1].start_offset, extent);
      styles[name] = s;

    } else if (line == "<pre id='vimCodeElement'>") {
      // We found the content itself. We are done.
      break;
    }

  }

  for (;;) {

    std::string line;
    if (!std::getline(html, line))
      break;

    if (line == "</pre>")
      break;

    // turn this line into ANSI highlighting
    std::string highlighted = from_html(styles, line);

    // yield the new line
    rc = callback(highlighted);
    if (rc != 0)
      goto done;
  }

  // success

done:
  (void)remove(output.c_str());

  return rc;
}

}

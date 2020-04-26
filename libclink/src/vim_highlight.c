#include <assert.h>
#include <clink/iter.h>
#include <clink/vim.h>
#include "colour.h"
#include <errno.h>
#include "iter.h"
#include "re.h"
#include <regex.h>
#include "run.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "temp_dir.h"
#include <unistd.h>

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

static int convert_to_html(const char *input, const char *output) {

  assert(input != NULL);
  assert(output != NULL);
  assert(strcmp(input, output) != 0);

  // construct a directive telling Vim to save to the given output path
  char *save_command = NULL;
  if (asprintf(&save_command, "+w %s", output) < 0)
    return errno;

  // Construct a command line to open the file in Vim, convert it to highlighted
  // HTML, save this to the output and exit. 
  // FIXME wrap it in timeout
  // in case the user has a weird ~/.vimrc and we end up hanging.
  char const *argv[] = { "vim", "-n", "+set nonumber", "+TOhtml", save_command,
    "+qa!", input, NULL };

  int rc = run(argv, true);

  free(save_command);
  return rc;
}

typedef struct {
  char *name;
  unsigned fg;
  unsigned bg;
  bool bold;
  bool underline;
} style_t;

// state used by the vim highlight iterator
typedef struct {

  // temporary directory we are working within
  char *dir;

  // temporary file within the above directory we outputted to
  char *output;

  // read handle to output
  FILE *highlighted;

  // last content line we yielded
  char *last;

  // styles that we have detected
  style_t *styles;
  size_t styles_size;

} state_t;

// clean up and deallocate a state_t
static void state_free(state_t **st) {

  if (st == NULL && *st == NULL)
    return;

  state_t *s = *st;

  for (size_t i = 0; i < s->styles_size; ++i) {
    free(s->styles[i].name);
    s->styles[i].name = NULL;
  }
  free(s->styles);
  s->styles = NULL;
  s->styles_size = 0;

  free(s->last);
  s->last = NULL;

  if (s->highlighted != NULL)
    (void)fclose(s->highlighted);
  s->highlighted = NULL;

  if (s->output != NULL)
    (void)unlink(s->output);
  free(s->output);
  s->output = NULL;

  if (s->dir != NULL)
    (void)rmdir(s->dir);
  free(s->dir);
  s->dir = NULL;

  free(s);
  *st = NULL;
}

// Decode a fragment of HTML text produced by Vim’s TOhtml. It is assumed that
// the input contains no HTML tags.
static int from_html(const state_t *s, const char *line, char **output) {

  assert(s->styles != NULL || s->styles_size == 0);
  assert(line != NULL);
  assert(output != NULL);

  // the HTML escapes produced by TOhtml
  struct translation {
    const char *key;
    char value;
  };
  static const struct translation HTML_DECODE[] = {
    { "amp;", '&' },
    { "gt;", '>' },
    { "lt;", '<' },
    { "nbsp;", ' ' },
    { "quot;", '"' },
  };
  static const size_t HTML_DECODE_SIZE
    = sizeof(HTML_DECODE) / sizeof(HTML_DECODE[0]);

  // setup a dynamic buffer to construct the output
  char *out = NULL;
  size_t out_size = 0;
  FILE *buf = open_memstream(&out, &out_size);
  if (buf == NULL)
    return errno;

#define PR(args...) \
  do { \
    if (fprintf(buf, args) < 0) { \
      int rc = errno; \
      (void)fclose(buf); \
      free(out); \
      return rc; \
    } \
  } while (0)
  

  for (size_t i = 0; i < strlen(line); i++) {

    static const char SPAN_OPEN[] = "<span class=\"";
    static const char SPAN_CLOSE[] = "</span>";

    if (line[i] == '&' && i + 1 < strlen(line)) {
      // An HTML escape; let us see if we can decode it.
      bool translated = false;
      for (size_t j = 0; j < HTML_DECODE_SIZE; j++) {
        const struct translation *t = &HTML_DECODE[j];
        if (strncmp(&line[i + 1], t->key, strlen(t->key)) == 0) {
          PR("%c", t->value);
          i += strlen(t->key);
          translated = true;
          break;
        }
      }

      if (translated)
        continue;
    }

    else if (line[i] == '<') {

      // is this a <span class="..."> ?
      if (strncmp(&line[i], SPAN_OPEN, strlen(SPAN_OPEN)) == 0) {
        size_t start = i + strlen(SPAN_OPEN);
        const char *end = strstr(&line[start], "\">");
        if (end != NULL) {
          size_t len = end - &line[start];
          bool formatted = false;
          for (size_t j = 0; j < s->styles_size; j++) {
            const style_t *style = &s->styles[j];
            if (strncmp(&line[start], style->name, len) == 0) {
              PR("\033[3%u;4%u%s%sm", style->fg, style->bg,
                (style->bold ? ";1" : ""), (style->underline ? ";4" : ""));
              i += end - &line[i] + 1;
              formatted = true;
              break;
            }
          }

          if (formatted)
            continue;
        }
      }
      
      // how about a </span> ?
      else if (strncmp(&line[i], SPAN_CLOSE, strlen(SPAN_CLOSE)) == 0) {
        PR("\033[0m");
        i += strlen(SPAN_CLOSE) - 1;
        continue;
      }
    }

    PR("%c", line[i]);
  }

#undef PR

  (void)fclose(buf);
  assert(out != NULL);
  *output = out;

  return 0;
}

// advance to the next content line
static int move_next(state_t *s) {

  assert(s != NULL);

  if (s->highlighted == NULL)
    return EINVAL;

  // clear the previous line
  free(s->last);
  s->last = NULL;

  int rc = 0;

  char *line = NULL;
  size_t line_size = 0;
  errno = 0;
  if (getline(&line, &line_size, s->highlighted) < 0) {
    rc = errno;

  // is this the end of the content?
  } else if (strcmp(line, "</pre>\n") == 0) {
    // leave it->last = NULL indicating iterator exhaustion

  } else {
    // turn this line into ANSI highlighting
    rc = from_html(s, line, &s->last);
  }

  free(line);

  return rc;
}

static int next(no_lookahead_iter_t *it, const char **yielded) {

  if (it == NULL)
    return EINVAL;

  if (yielded == NULL)
    return EINVAL;

  state_t *s = it->state;

  if (s == NULL)
    return EINVAL;

  // find the next-to-yield line
  int rc = move_next(s);
  if (rc)
    return rc;

  // is the iterator exhausted?
  if (s->last == NULL)
    return ENOMSG;

  // extract the next-to-yield line
  *yielded = s->last;

  return rc;
}

static void my_free(no_lookahead_iter_t *it) {

  if (it == NULL)
    return;

  state_free((state_t**)&it->state);
}

int clink_vim_highlight(clink_iter_t **it, const char *filename) {

  if (it == NULL)
    return EINVAL;

  if (filename == NULL)
    return EINVAL;

  // validate that the file exists and is readable
  if (access(filename, R_OK) < 0)
    return errno;

  // create state that we will maintain within the iterator
  state_t *s = calloc(1, sizeof(*s));
  if (s == NULL)
    return ENOMEM;

  no_lookahead_iter_t *i = NULL;
  clink_iter_t *wrapper = NULL;
  int rc = 0;

  // create a temporary directory to use for scratch space
  if ((rc = temp_dir(&s->dir)))
    goto done;

  // construct a path to a temporary file to use for output
  if (asprintf(&s->output, "%s/temp.html", s->dir) < 0) {
    rc = errno;
    goto done;
  }

  // convert the input to highlighted HTML
  if ((rc = convert_to_html(filename, s->output)))
    goto done;

  // open the generated HTML
  s->highlighted = fopen(s->output, "r");
  if (s->highlighted == NULL) {
    rc = errno;
    goto done;
  }

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
  regex_t style_re;
  if ((rc = regcomp(&style_re, STYLE, REG_EXTENDED))) {
    rc = re_err_to_errno(rc);
    goto done;
  }

  char *line = NULL;
  size_t line_size = 0;
  for (;;) {

    if (getline(&line, &line_size, s->highlighted) < 0) {
      rc = errno;
      free(line);
      if (rc)
        goto done1;
      break;
    }

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
      regmatch_t m[10];
      size_t m_size = sizeof(m) / sizeof(m[0]);
      int r = regexec(&style_re, line, m_size, m, 0);
      if (r == REG_NOMATCH) {
        continue;
      } else if (r != 0) {
        rc = re_err_to_errno(r);
        free(line);
        goto done1;
      }

      // build a style defining these attributes
      style_t style = { .fg = 9, .bg = 9, .bold = false, .underline = false };
      if (m[2].rm_so >= 0)
        style.fg = html_colour_to_ansi(line + m[3].rm_so);
      if (m[4].rm_so >= 0)
        style.bg = html_colour_to_ansi(line + m[5].rm_so);
      if (m[7].rm_so >= 0)
        style.bold = true;
      if (m[9].rm_so >= 0)
        style.underline = true;

      // extract the name of this style
      size_t extent = m[1].rm_eo - m[1].rm_so;
      style.name = strndup(line + m[1].rm_so, extent);
      if (style.name == NULL) {
        rc = errno;
        free(line);
        goto done1;
      }

      // expand the styles collection to make room for this new entry
      style_t *styles
        = realloc(s->styles, sizeof(styles[0]) * (s->styles_size + 1));
      if (styles == NULL) {
        rc = ENOMEM;
        free(style.name);
        free(line);
        goto done1;
      }
      s->styles = styles;
      ++s->styles_size;

      // register this style for later highlighting
      s->styles[s->styles_size - 1] = style;

    } else if (strcmp(line, "<pre id='vimCodeElement'>\n") == 0) {
      // We found the content itself. We are done.
      free(line);
      break;
    }
  }

  // if we reached here, we successfully parsed the style list and are into the
  // content, so now create an iterator for the caller
  i = calloc(1, sizeof(*i));
  if (i == NULL) {
    rc = ENOMEM;
    goto done1;
  }

  i->next_str = next;
  i->state = s;
  s = NULL;
  i->free = my_free;

  // wrap our no-lookahead iterator in a 1-lookahead iterator
  if ((rc = iter_new(&wrapper, i)))
    goto done1;

done1:
  regfree(&style_re);
done:
  if (rc) {
    clink_iter_free(&wrapper);
    no_lookahead_iter_free(&i);
    state_free(&s);
  } else {
    *it = wrapper;
  }

  return rc;
}

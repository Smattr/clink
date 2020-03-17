#include <assert.h>
#include <clink/vim.h>
#include "colour.h"
#include <errno.h>
#include "error.h"
#include "list.h"
#include <regex.h>
#include "run.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int clink_vim_open(const char *filename, unsigned long lineno,
    unsigned long colno) {

  assert(filename != NULL);

  // check line number is valid
  if (lineno == 0)
    return ERANGE;

  // check column number is valid
  if (colno == 0)
    return ERANGE;

  // construct a directive telling Vim to jump to the given position
  char cursor[128];
  snprintf(cursor, sizeof(cursor), "+call cursor(%lu,%lu)", lineno, colno);

  // construct a argument vector to invoke Vim
  char const *argv[] = { "vim", cursor, filename, NULL };

  // run it
  return run(argv, false);
}

// The remainder of this file is essentially insanity. Do not say I did not
// warn you.
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

  int rc = -1;

  // construct a directive telling Vim to save to the given output path
  char *save = NULL;
  if (asprintf(&save, "+w %s", output) < 0) {
    rc = ENOMEM;
    goto done;
  }

  // Construct a command line to open the file in Vim, convert it to highlighted
  // HTML, save this to the output and exit. 
  // FIXME wrap it in timeout
  // in case the user has a weird ~/.vimrc and we end up hanging.
  char const *argv[] = { "vim", "-n", "+set nonumber", "+TOhtml", save, "+qa!",
    input, NULL };

  rc = run(argv, true);

done:
  free(save);

  return rc;
}

struct style {
  unsigned fg;
  unsigned bg;
  bool bold;
  bool underline;
};

struct style_list {
  char **name;
  struct style *style;
  size_t size;
  size_t capacity;
};

static int style_list_expand(struct style_list *list) {

  assert(list != NULL);

  size_t cap = list->capacity == 0 ? 1 : list->capacity * 2;

  // expand names
  char **n = realloc(list->name, cap * sizeof(n[0]));
  if (n == NULL)
    return ENOMEM;
  list->name = n;

  // expand styles
  struct style *st = realloc(list->style, cap * sizeof(st[0]));
  if (st == NULL)
    return ENOMEM;
  list->style = st;

  // success
  list->capacity = cap;

  return 0;
}

static int style_list_add(struct style_list *list, const char *name,
    size_t name_len, struct style s) {

  assert(list != NULL);
  assert(name != NULL);

  // expand styles collection if necessary
  if (list->size == list->capacity) {
    int r = style_list_expand(list);
    if (r != 0)
      return r;
  }

  // construct the name of this style
  char *n = NULL;
  if (asprintf(&n, "%.*s", (int)name_len, name) < 0)
    return ENOMEM;

  assert(list->size < list->capacity);
  size_t index = list->size;

  list->name[index] = n;
  list->style[index] = s;
  list->size++;

  return 0;
}

static void style_list_clear(struct style_list *list) {

  assert(list != NULL);

  for (size_t i = 0; i < list->size; i++)
    free(list->name[i]);
  free(list->name);
  free(list->style);

  list->size = list->capacity = 0;
}

// Decode a fragment of HTML text produced by Vim’s TOhtml. It is assumed that
// the input contains no HTML tags.
static int from_html(const struct style_list *styles, const char *line,
    char **highlighted) {

  assert(styles != NULL);
  assert(line != NULL);
  assert(highlighted != NULL);

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

  int rc = 0;

  // setup a dynamic buffer to construct the output
  char *result = NULL;
  size_t result_size = 0;
  FILE *f = open_memstream(&result, &result_size);
  if (f == NULL) {
    rc = errno;
    goto done;
  }

#define PR(args...) \
  do { \
    if (fprintf(f, ## args) < 0) { \
      rc = ENOMEM; \
      goto done; \
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
        char *end = strstr(&line[start], "\">");
        if (end != NULL) {
          size_t extent = end - &line[start];
          bool found = false;
          for (size_t j = 0; j < styles->size; j++) {
            if (extent != strlen(styles->name[j]))
              continue;
            if (strncmp(&line[start], styles->name[j], extent) == 0) {
              PR("\033[3%u;4%u", styles->style[j].fg, styles->style[j].bg);
              if (styles->style[j].bold)
                PR(";1");
              if (styles->style[j].underline)
                PR(";4");
              PR("m");
              i += strlen(SPAN_OPEN) + extent + 1;
              found = true;
              break;
            }
          }
          if (found)
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

done:
  // finalise the dynamic buffer
  if (f != NULL)
    fclose(f);

  if (rc == 0) {
    *highlighted = result;
  } else {
    free(result);
  }

  return rc;
}

static int mktmp(char **temp) {

  assert(temp != NULL);

  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";

  if (asprintf(temp, "%s/tmp.XXXXXX", TMPDIR) < 0)
    return ENOMEM;

  if (mkdtemp(*temp) == NULL) {
    free(*temp);
    *temp = NULL;
    return errno;
  }

  return 0;
}

int clink_vim_highlight(const char *filename, char ***lines, size_t *lines_size) {

  assert(filename != NULL);
  assert(lines != NULL);
  assert(lines_size != NULL);

  // accrue lines here that we will later assign to lines and lines_size
  list_t sl = { 0 };

  FILE *html = NULL;

  // create a temporary directory to use for scratch space
  char *temp = NULL;
  int rc = mktmp(&temp);
  if (rc != 0)
    return rc;

  // convert the input to highlighted HTML
  char *output = NULL;
  if (asprintf(&output, "%s/temp.html", temp) < 0) {
    rc = errno;
    goto done;
  }
  if ((rc = convert_to_html(filename, output)))
    goto done;

  // open the generated HTML
  html = fopen(output, "r");
  if (html == NULL) {
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

  regex_t style;
  static const char STYLE[] = "^\\.([[:alpha:]][[:alnum:]]+)[[:blank:]]*"
    "\\{[[:blank:]]*(color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*)?"
    "(background-color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*"
    "(padding-bottom:[[:blank:]]*1px;[[:blank:]]*)?)?"
    "(font-weight:[[:blank:]]*bold;[[:blank:]]*)?"
    "(font-style:[[:blank:]]*italic;[[:blank:]]*)?"
    "(text-decoration:[[:blank:]]*underline;[[:blank:]]*)?";
  if ((rc = regcomp(&style, STYLE, REG_EXTENDED))) {
    rc = regex_error(rc);
    goto done;
  }

  char *line = NULL;
  size_t line_size = 0;
  struct style_list styles = { 0 };

  for (;;) {

    if (getline(&line, &line_size, html) == -1)
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
      regmatch_t match[10];
      static const size_t NMATCH = sizeof(match) / sizeof(match[0]);

      // extract CSS attributes from this line
      if (regexec(&style, line, NMATCH, match, 0) == REG_NOMATCH)
        continue;

      // build a style defining these attributes
      struct style s = { .fg = 9, .bg = 9, .bold = false, .underline = false };
      if (match[2].rm_so != -1)
        s.fg = html_colour_to_ansi(&line[match[3].rm_so], 6);
      if (match[4].rm_so != -1)
        s.bg = html_colour_to_ansi(&line[match[5].rm_so], 6);
      if (match[7].rm_so != -1)
        s.bold = true;
      if (match[9].rm_so != -1)
        s.underline = true;

      // register this style for later highlighting
      const char *name_start = line + match[1].rm_so;
      size_t name_extent = match[1].rm_eo - match[1].rm_so;
      if ((rc = style_list_add(&styles, name_start, name_extent, s)))
        goto done1;

    } else if (strcmp(line, "<pre id='vimCodeElement'>\n") == 0) {
      // We found the content itself. We are done.
      break;
    }

  }

  for (;;) {

    if (getline(&line, &line_size, html) == -1)
      break;

    if (strcmp(line, "</pre>\n") == 0)
      break;

    // turn this line into ANSI highlighting
    char *highlighted = NULL;
    if ((rc = from_html(&styles, line, &highlighted)))
      goto done1;

    // add the new line
    if ((rc = list_append(&sl, highlighted))) {
      free(highlighted);
      goto done1;
    }
  }

  // success

done1:
  // clean up
  style_list_clear(&styles);
  free(line);
  regfree(&style);

done:
  // more clean up
  if (html != NULL)
    fclose(html);
  if (output != NULL) {
    (void)remove(output);
    free(output);
  }
  if (temp != NULL) {
    (void)rmdir(temp);
    free(temp);
  }

  if (rc == 0) {
    *lines = (char**)sl.data;
    *lines_size = sl.size;
  } else {
    list_free(&sl, NULL);
  }

  return rc;
}

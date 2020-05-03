#include "colour.h"
#include <curses.h>
#include <stddef.h>
#include <stdio.h>

// we need to pack a colour combination in this way because the default ncurses
// implementation only supports 64 colour pairs and hence rejects any colour ID
// above 64
static short colour_pair_id(short fg, short bg) {
  return ((fg << 3) | bg) + 1;
}

int init_ncurses_colours(void) {

  // this function is only expected to be called when we know we have colour
  // support
  if (!has_colors())
    return -1;

  if (start_color() != 0)
    return -1;

  // make the current terminal colour scheme available
  use_default_colors();

  static const short COLOURS[] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN,
    COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };

  // use a simple encoding scheme to configure every possible colour
  // combination
  for (size_t i = 0; i < sizeof(COLOURS) / sizeof(COLOURS[0]); ++i) {
    short fg = COLOURS[i];
    for (size_t j = 0; j < sizeof(COLOURS) / sizeof(COLOURS[0]); ++j) {
      short bg = COLOURS[j];
      short id = colour_pair_id(fg, bg);
      if (init_pair(id, fg, bg) != 0) {
        return -1;
      }
    }
  }

  return 0;
}

static int print_bw(const char *s, int (*put)(int c, FILE *stream),
  FILE *stream) {

  enum {
    IDLE,
    SAW_ESC,
    SAW_LSQUARE,
  } state = IDLE;

  for (; *s != '\0'; ++s) {

    switch (state) {

      case IDLE:
        if (*s == 27) {
          state = SAW_ESC;
        } else {
          if (put(*s, stream) == EOF)
            return -1;
        }
        break;

      case SAW_ESC:
        if (*s == '[') {
          state = SAW_LSQUARE;
        } else {
          if (put(*s, stream) == EOF)
            return -1;
          state = IDLE;
        }
        break;

      case SAW_LSQUARE:
        if (*s == 'm')
          state = IDLE;
        break;

    }

  }

  return 0;
}

int printf_bw(const char *s, int (*put)(int c, FILE *stream), FILE *stream) {
  return print_bw(s, put, stream);
}

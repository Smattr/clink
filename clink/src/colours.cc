#include <cstddef>
#include <cassert>
#include "colours.h"
#include <cstdint>
#include <ctype.h>
#include <limits.h>
#include <ncurses.h>
#include <string>

// We need to pack a colour combination in this way because the default ncurses
// implementation only supports 64 colour pairs and hence rejects any colour ID
// above 64.
static constexpr short colour_pair_id(short fg, short bg) {
  return ((fg << 3) | bg) + 1;
}

int init_ncurses_colours() {

  // this function is only expected to be called when we know we have colour
  // support
  assert(has_colors());

  if (start_color() != 0)
    return -1;

  // make the current terminal colour scheme available
  use_default_colors();

  static const short COLOURS[] = { COLOR_BLACK, COLOR_RED, COLOR_GREEN,
    COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE };

  // use a simple encoding scheme to configure every possible colour
  // combination.
  for (const short &fg : COLOURS) {
    for (const short &bg : COLOURS) {
      short id = colour_pair_id(fg, bg);
      if (init_pair(id, fg, bg) != 0) {
        return -1;
      }
    }
  }

  return 0;
}

void printw_in_colour(const std::string &text) {

  enum {
    IDLE,
    SAW_ESC,
    SAW_LSQUARE,
  } state = IDLE;

  // a partial ANSI code we have parsed
  std::string pending_code;

  // pending attributes that we have accrued while parsing
  bool bold;
  bool underline;
  short fg;
  short bg;

  for (const char &c : text) {

    switch (state) {

      case IDLE:
        if (c == 27) {
          state = SAW_ESC;
        } else {
          addch(c);
        }
        break;

      case SAW_ESC:
        if (c == '[') {
          bold = false;
          underline = false;
          fg = COLOR_WHITE;
          bg = COLOR_BLACK;
          pending_code = "";
          state = SAW_LSQUARE;
        } else {
          addch(c);
          state = IDLE;
        }
        break;

      case SAW_LSQUARE:

        if (c == ';' || c == 'm') {
          if (pending_code != "") {
            int code = stoi(pending_code);
            pending_code = "";
            if (code == 1) {
              bold = true;
            } else if (code == 4) {
              underline = true;
            } else if (code >= 30 && code <= 37) {
              fg = code - 30;
            } else if (code >= 40 && code <= 47) {
              bg = code - 40;
            } else if (code == 0) { // reset
              standend();
              state = IDLE;
              continue;
            }
            // otherwise, ignore
          }
        }

        if (c == 'm') {
          attrset((bold ? A_BOLD : 0) | (underline ? A_UNDERLINE : 0) |
            COLOR_PAIR(colour_pair_id(fg, bg)));
          state = IDLE;
        } else if (c >= '0' && c <= '9') {
          pending_code += c;
        } else if (c == ';') {
          // nothing
        } else {
          // something unrecognised
          addch(c);
          state = IDLE;
        }

        break;

    }
  }
}

void printw_in_bw(const std::string &s) {

  enum {
    IDLE,
    SAW_ESC,
    SAW_LSQUARE,
  } state = IDLE;

  for (const char &c : s) {

    switch (state) {

      case IDLE:
        if (c == 27) {
          state = SAW_ESC;
        } else {
          addch(c);
        }
        break;

      case SAW_ESC:
        if (c == '[') {
          state = SAW_LSQUARE;
        } else {
          addch(c);
          state = IDLE;
        }
        break;

      case SAW_LSQUARE:
        if (c == 'm')
          state = IDLE;
        break;

    }

  }
}

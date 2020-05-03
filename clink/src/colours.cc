#include <cstddef>
#include <cassert>
#include "colours.h"
#include <cstdint>
#include <ctype.h>
#include <limits.h>
#include <ncurses.h>
#include <string>

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

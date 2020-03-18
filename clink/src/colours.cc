#include <array>
#include <cassert>
#include "colours.h"
#include <cstdint>
#include <ctype.h>
#include <limits.h>
#include "log.h"
#include <ncurses.h>
#include <string>

using namespace std;

/* We need to pack a colour combination in this way because the default ncurses
 * implementation only supports 64 colour pairs and hence rejects any colour ID
 * above 64.
 */
static constexpr short colour_pair_id(short fg, short bg) {
  return ((fg << 3) | bg) + 1;
}

int init_ncurses_colours() {

  /* This function is only expected to be called when we know we have colour
   * support.
   */
  assert(has_colors());

  if (start_color() != 0)
    return -1;

  // Make the current terminal colour scheme available.
  use_default_colors();

  LOG("COLORS == %d", COLORS);
  LOG("COLOR_PAIRS == %d", COLOR_PAIRS);

  static const array<short, 8> COLOURS = { { COLOR_BLACK, COLOR_RED,
    COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN,
    COLOR_WHITE } };

  /* Use a simple encoding scheme to configure every possible colour
   * combination.
   */
  for (const short &fg : COLOURS) {
    for (const short &bg : COLOURS) {
      short id = colour_pair_id(fg, bg);
      if (init_pair(id, fg, bg) != 0) {
        LOG("failed to init colour pair %hd", id);
        return -1;
      }
    }
  }

  return 0;
}

void printw_in_colour(const string &text) {

  enum {
    IDLE,
    SAW_ESC,
    SAW_LSQUARE,
  } state = IDLE;

  // A partial ANSI code we've parsed.
  string pending_code;

  // Pending attributes that we've accrued while parsing.
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
              LOG("resetting ncurses colours");
              standend();
              state = IDLE;
              continue;
            }
            // otherwise, ignore
          }
        }

        if (c == 'm') {
          LOG("switching to ncurses fg = %hd, bg = %hd, %sbold, %sunderline",
            fg, bg, bold ? "" : "no ", underline ? "" : "no ");
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

string strip_ansi(const string &s) {

  enum {
    IDLE,
    SAW_ESC,
    SAW_LSQUARE,
  } state = IDLE;

  string output;

  for (const char &c : s) {

    switch (state) {

      case IDLE:
        if (c == 27) {
          state = SAW_ESC;
        } else {
          output += c;
        }
        break;

      case SAW_ESC:
        if (c == '[') {
          state = SAW_LSQUARE;
        } else {
          output += c;
          state = IDLE;
        }
        break;

      case SAW_LSQUARE:
        if (c == 'm')
          state = IDLE;
        break;

    }

  }

  return output;
}

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

static uint8_t hex_to_int(char c) {
  assert(isxdigit(c));
  switch (c) {
    case '0' ... '9': return c - '0';
    case 'a' ... 'f': return uint8_t(c - 'a') + 10;
    case 'A' ... 'F': return uint8_t(c - 'A') + 10;
  }
  __builtin_unreachable();
}

unsigned html_colour_to_ansi(const char *html, [[gnu::unused]] size_t length) {
  assert(length == 6);

  // Extract the color as RGB.
  uint8_t red = hex_to_int(html[0]) * 16 + hex_to_int(html[1]);
  uint8_t green = hex_to_int(html[2]) * 16 + hex_to_int(html[3]);
  uint8_t blue = hex_to_int(html[4]) * 16 + hex_to_int(html[5]);

  /* HTML has a 24-bit color space, but ANSI color codes have an 8-bit color
   * space. We map an HTML color onto an ANSI color by finding the "closest" one
   * using an ad hoc notion of distance between colors.
   */

  /* First, we define the ANSI colors as RGB values. These definitions match
   * what 2html uses for 8-bit color, so an HTML color intended to map
   * *exactly* to one of these should correctly end up with a distance of 0.
   */
  static const array<uint8_t[3], 8> ANSI { {
    /* black   */ { 0x00, 0x00, 0x00 },
    /* red     */ { 0xff, 0x60, 0x60 },
    /* green   */ { 0x00, 0xff, 0x00 },
    /* yellow  */ { 0xff, 0xff, 0x00 },
    /* blue    */ { 0x80, 0x80, 0xff },
    /* magenta */ { 0xff, 0x40, 0xff },
    /* cyan    */ { 0x00, 0xff, 0xff },
    /* white   */ { 0xff, 0xff, 0xff },
  } };

  // Now find the color with the least distance to the input.
  size_t min_index;
  unsigned min_distance = UINT_MAX;
  for (size_t i = 0; i < ANSI.size(); i++) {
    unsigned distance =
      (ANSI[i][0] > red ? ANSI[i][0] - red : red - ANSI[i][0]) +
      (ANSI[i][1] > green ? ANSI[i][1] - green : green - ANSI[i][1]) +
      (ANSI[i][2] > blue ? ANSI[i][2] - blue : blue - ANSI[i][2]);
    if (distance < min_distance) {
      min_index = i;
      min_distance = distance;
    }
  }

  return unsigned(min_index);
}

int init_ncurses_colours() {

  /* This function is only expected to be called when we know we have colour
   * support.
   */
  assert(has_colors());

  // Make the current terminal colour scheme available.
  use_default_colors();

  static const array<short, 9> COLOURS = { { COLOR_BLACK, COLOR_RED,
    COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN,
    COLOR_WHITE, -1 } };

  /* Use a simple encoding scheme to configure every possible colour
   * combination.
   */
  for (const short &fg : COLOURS) {
    for (const short &bg : COLOURS) {
      short id = (fg << 8) | bg;
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
          fg = -1;
          bg = -1;
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
              fg = -1;
              bg = -1;
              attron(A_NORMAL | COLOR_PAIR((fg << 8) | bg));
              state = IDLE;
              continue;
            }
            // otherwise, ignore
          }
        }

        if (c == 'm') {
          attron((bold ? A_BOLD : 0) | (underline ? A_UNDERLINE : 0) |
            COLOR_PAIR((fg << 8) | bg));
          state = IDLE;
        } else if (c >= '0' && c <= '9') {
          pending_code += c;
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

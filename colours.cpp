#include <array>
#include <cassert>
#include "colours.h"
#include <cstdint>
#include <ctype.h>
#include <limits.h>

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

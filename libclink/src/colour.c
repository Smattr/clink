#include <assert.h>
#include "colour.h"
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static uint8_t hex_to_int(char c) {
  assert(isxdigit(c));
  switch (c) {
    case '0' ... '9': return c - '0';
    case 'a' ... 'f': return (uint8_t)(c - 'a') + 10;
    case 'A' ... 'F': return (uint8_t)(c - 'A') + 10;
  }
  __builtin_unreachable();
}

static unsigned diff(uint8_t a, uint8_t b) {
  if (a > b)
    return a - b;
  return b - a;
}

unsigned html_colour_to_ansi(const char *html) {

  // extract the colour as RGB
  uint8_t red = hex_to_int(html[0]) * 16 + hex_to_int(html[1]);
  uint8_t green = hex_to_int(html[2]) * 16 + hex_to_int(html[3]);
  uint8_t blue = hex_to_int(html[4]) * 16 + hex_to_int(html[5]);

  // HTML has a 24-bit colour space, but ANSI colour codes have an 8-bit colour
  // space. We map an HTML colour onto an ANSI colour by finding the “closest”
  // one using an ad hoc notion of distance between colours.

  // First, we define the ANSI colours as RGB values. These definitions match
  // what 2html uses for 8-bit colour, so an HTML colour intended to map
  // *exactly* to one of these should correctly end up with a distance of 0.
  struct colour { uint8_t red; uint8_t green; uint8_t blue; };
  static const struct colour ANSI[] = {
    /* black   */ { 0x00, 0x00, 0x00 },
    /* red     */ { 0xff, 0x60, 0x60 },
    /* green   */ { 0x00, 0xff, 0x00 },
    /* yellow  */ { 0xff, 0xff, 0x00 },
    /* blue    */ { 0x80, 0x80, 0xff },
    /* magenta */ { 0xff, 0x40, 0xff },
    /* cyan    */ { 0x00, 0xff, 0xff },
    /* white   */ { 0xff, 0xff, 0xff },
  };
  static const size_t ANSI_SIZE = sizeof(ANSI) / sizeof(ANSI[0]);

  // now find the colour with the least distance to the input
  size_t min_index;
  unsigned min_distance = UINT_MAX;
  for (size_t i = 0; i < ANSI_SIZE; i++) {
    unsigned distance = diff(red, ANSI[i].red)
                      + diff(green, ANSI[i].green)
                      + diff(blue, ANSI[i].blue);
    if (distance < min_distance) {
      min_index = i;
      min_distance = distance;
    }
  }

  return (unsigned)min_index;
}

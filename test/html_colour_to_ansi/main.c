// test html_colour_to_ansi()

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "colour.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {

  // iterate over all HTML colours
  for (unsigned r = 0; r < 256; r++) {
    for (unsigned g = 0; g < 256; g++) {
      for (unsigned b = 0; b < 256; b++) {

        // construct the HTML colour string
        char html[7];
        {
          int rc = snprintf(html, sizeof(html), "%02x%02x%02x", r, g, b);
          assert(rc == sizeof(html) - 1);
        }

        // translate it to an ANSI colour
        unsigned ansi = html_colour_to_ansi(html);

        // check this is a valid ANSI colour
        assert(ansi < 8);

        // if this was exactly an ANSI colour, confirm we got the correct one
        if (r == 0x00 && g == 0x00 && b == 0x00) {
          assert(ansi == 0);
        } else if (r == 0xff && g == 0x60 && b == 0x60) {
          assert(ansi == 1);
        } else if (r == 0x00 && g == 0xff && b == 0x00) {
          assert(ansi == 2);
        } else if (r == 0xff && g == 0xff && b == 0x00) {
          assert(ansi == 3);
        } else if (r == 0x80 && g == 0x80 && b == 0xff) {
          assert(ansi == 4);
        } else if (r == 0xff && g == 0x40 && b == 0xff) {
          assert(ansi == 5);
        } else if (r == 0x00 && g == 0xff && b == 0xff) {
          assert(ansi == 6);
        } else if (r == 0xff && g == 0xff && b == 0xff) {
          assert(ansi == 7);
        }
      }
    }
  }

  return EXIT_SUCCESS;
}

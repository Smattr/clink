#include "colour.h"
#include <stdio.h>

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

int printf_bw(const char *s, FILE *stream) {
  return print_bw(s, fputc, stream);
}

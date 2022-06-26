#include "debug.h"
#include <clink/debug.h>
#include <stddef.h>
#include <stdio.h>
#include <vimcat/vimcat.h>

FILE *debug;

FILE *clink_set_debug(FILE *stream) {
  (void)vimcat_set_debug(stream);

  FILE *old = debug;
  debug = stream;
  return old;
}

void clink_debug_on(void) { (void)clink_set_debug(stderr); }

void clink_debug_off(void) { (void)clink_set_debug(NULL); }

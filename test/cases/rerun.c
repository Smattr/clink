/// when rescanning an unmodified file, Clink should be clever enough to skip it

void foo(void) {
  int x = 42;
}

// XFAIL: True
// RUN: clink --build-only --database={tmp} --debug {__file__} >/dev/null
// RUN: clink --build-only --colour=never --database={tmp} --debug --jobs=1 {__file__} 2>&1 | grep --colour=never "skipping unmodified file {__file__}"
// CHECK: 0: skipping unmodified file {__file__}

// doing this a third time should also hit the cache
// RUN: clink --build-only --colour=never --database={tmp} --debug --jobs=1 {__file__} 2>&1 | grep --colour=never "skipping unmodified file {__file__}"
// CHECK: 0: skipping unmodified file {__file__}

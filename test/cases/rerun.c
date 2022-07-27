/// when rescanning an unmodified file, Clink should be clever enough to skip it

void foo(void) {
  int x = 42;
}

// RUN: clink --build-only --database={%t} --debug {%s} >/dev/null
// RUN: clink --build-only --colour=never --database={%t} --debug --jobs=1 {%s} 2>&1 | grep --colour=never "skipping unmodified file {%s}"
// CHECK: 0: skipping unmodified file {%s}

// doing this a third time should also hit the cache
// RUN: clink --build-only --colour=never --database={%t} --debug --jobs=1 {%s} 2>&1 | grep --colour=never "skipping unmodified file {%s}"
// CHECK: 0: skipping unmodified file {%s}

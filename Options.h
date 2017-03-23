#pragma once

typedef enum {
  UI_NONE,
  UI_CURSES,
  UI_LINE,
} ui_t;

struct Options {
  const char *database;
  bool update_database;
  ui_t ui;

  // Parallelism (0 == auto).
  unsigned long threads;
};

extern Options opts;

#pragma once

// open Vim at the given position in the given file
//
// Returns Vim’s exit status.
int clink_vim_open(const char *filename, unsigned long lineno,
  unsigned long colno);

// yield a list of lines from the given file, syntax-highlighted using ANSI
// colour sequences, as if by Vim
int clink_vim_highlight(const char *filename, int (*callback)(const char *line));

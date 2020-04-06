#pragma once

#include <array>
#include <cstddef>
#include <clink/clink.h>
#include <signal.h>
#include <string>
#include <vector>

typedef enum {
  UICS_INPUT,
  UICS_ROWSELECT,
  UICS_EXITING,
} uicurses_state_t;

struct Results;

struct ResultRow {
  std::array<std::string, 4> text;
  std::string path;
  unsigned long line;
  unsigned long col;
};

class UICurses {

 public:
  int run(clink::Database &db);
  UICurses();
  ~UICurses();

 private:
  void move_to_line_no_blank(unsigned target);
  void move_to_line(unsigned target);
  void handle_input(clink::Database &db);
  void handle_select();

 private:
  uicurses_state_t state = UICS_INPUT;
  std::string left = "", right = "";
  unsigned m_index = 0, x, y, select_index = 0;
  int ret;
  std::vector<ResultRow> results;
  unsigned from_row = 0;
  struct sigaction original_sigtstp_handler;
  struct sigaction original_sigwinch_handler;
  bool color;

};

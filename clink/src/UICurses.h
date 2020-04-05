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

#define HEADINGS_SZ 4

struct ResultRow {
  std::array<std::string, HEADINGS_SZ> text;
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
  uicurses_state_t m_state = UICS_INPUT;
  std::string m_left = "", m_right = "";
  unsigned m_index = 0, m_x, m_y, m_select_index = 0;
  int m_ret;
  std::vector<ResultRow> m_results;
  unsigned m_from_row = 0;
  struct sigaction m_original_sigtstp_handler;
  struct sigaction m_original_sigwinch_handler;
  bool m_color;

};

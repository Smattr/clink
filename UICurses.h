#pragma once

#include "Database.h"
#include <signal.h>
#include <string>
#include "UI.h"

typedef enum {
  UICS_INPUT,
  UICS_ROWSELECT,
  UICS_EXITING,
} uicurses_state_t;

struct Results;

class UICurses : public UI {

 public:
  int run(Database &db) final;
  UICurses();
  ~UICurses();

 private:
  void move_to_line_no_blank(unsigned target);
  void move_to_line(unsigned target);
  void handle_input(Database &db);
  void handle_select();

 private:
  uicurses_state_t m_state = UICS_INPUT;
  std::string m_left = "", m_right = "";
  unsigned m_index = 0, m_x, m_y, m_select_index = 0;
  int m_ret;
  Results *m_results = nullptr;
  unsigned m_from_row = 0;
  struct sigaction m_original_sigtstp_handler;
  bool m_color;

};

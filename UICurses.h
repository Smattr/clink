#pragma once

#include "Database.h"
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
    int run(Database &db) override;
    UICurses();
    ~UICurses();

private:
    void move_to_line(unsigned target);
    void handle_input(Database &db);
    void handle_select();

private:
    uicurses_state_t m_state;
    std::string m_left, m_right;
    unsigned m_index, m_x, m_y, m_select_index;
    int m_ret;
    Results *m_results;
    unsigned m_from_row;

};

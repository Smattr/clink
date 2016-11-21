#pragma once

#include <pthread.h>
#include <string>
#include <vector>

enum TermColour {
    TC_BLACK = 0,
    TC_RED = 1,
    TC_GREEN = 2,
    TC_YELLOW = 3,
    TC_BLUE = 4,
    TC_MAGENTA = 5,
    TC_CYAN = 6,
    TC_WHITE = 7,
};

struct TermChar {
    int c;
    TermColour fg, bg;
    bool bold;
    bool underline;

    std::string to_string() const {
        if (c == 0)
            return " ";
        std::string s("\033[3");
        s += std::to_string(fg) + ";4" + std::to_string(bg);
        if (bold)
            s += ";1";
        if (underline)
            s += ";4";
        s += "m" + std::to_string((char)c) + "\033[0m";
        return s;
    }
};

class FauxTerm {

public:
    FauxTerm();
    ~FauxTerm();

    int get_pipe() const {
        return m_pipe_fd[1];
    }

    TermChar get_char(unsigned x, unsigned y) const;
    std::vector<TermChar> get_line(unsigned y) const;

private:
    unsigned m_width, m_height;
    int m_pipe_fd[2], m_sig_fd;
    TermChar *m_screen;
    pthread_t m_child;

    friend void *bridge(void *state);

};

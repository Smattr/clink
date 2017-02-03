#pragma once

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

int vim_open(const std::string &filename, unsigned line, unsigned col);

class VimHighlighter {

public:
    VimHighlighter(const std::string &filename);
    ~VimHighlighter();
    std::string get_line(unsigned lineno);

private:
    struct Style {
        unsigned fg;
        unsigned bg;
        bool bold;
        bool underline;
    };

    std::string m_tempdir;
    FILE *m_html = nullptr;
    char *m_last_line = nullptr;
    size_t m_last_line_sz = 0;

    std::unordered_map<std::string, Style> m_styles;
    std::vector<std::string> m_lines;

    std::string from_html(const std::string &text);
};

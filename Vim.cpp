#include <array>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <limits.h>
#include <regex.h>
#include <string>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "Vim.h"
#include <vector>

// FIXME: Use a safer method of calling Vim that doesn't choke on paths with spaces etc

using namespace std;

int vim_open(const string &filename, unsigned line, unsigned col) {
    string cmd = "vim \"+call cursor(" + to_string(line) + "," + to_string(col)
        + ")\" " + filename;
    return system(cmd.c_str());
}

#ifdef TEST_VIM_OPEN
static void usage(const string &prog) {
    cerr << "usage: " << prog << " filename line col\n";
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const string filename = argv[1];
    unsigned line, col;

    try {
        line = stoul(argv[2]);
        col = stoul(argv[3]);
    } catch (exception &e) {
        cerr << e.what() << "\n";
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    return vim_open(filename, line, col);
}
#endif

/******************************************************************************
 *                                                                            *
 * The remainder of this file is essentially insanity. Don't say I didn't     *
 * warn you.                                                                  *
 *                                                                            *
 * ----                                                                       *
 *                                                                            *
 * We would like to display the contents of a file in a terminal, with syntax *
 * highlighting as if it were in Vim. This isn't as straightforward as it     *
 * sounds because a user's settings are not known (e.g. they might have       *
 * extended a language with further keywords). There are two obvious ideas    *
 * for how to achieve this:                                                   *
 *                                                                            *
 *   1. Teach Clink how to parse ~/.vimrc; or                                 *
 *   2. Call out to Vimcat.                                                   *
 *                                                                            *
 * Parsing ~/.vimrc is almost an immediate non-starter. The Vim configuration *
 * syntax is complex, highly dynamic and not well documented. Parsing a       *
 * user's Vim settings correctly is likely to involve more complexity than    *
 * the rest of Clink combined.                                                *
 *                                                                            *
 * Using Vimcat is attractive, but despite my best efforts I haven't been     *
 * able to get any of the variants of Vimcat floating around on the internet  *
 * to work reliably.                                                          *
 *                                                                            *
 * So what is one to do? Well, brood on the problem for a while before        *
 * landing on an idea that so crazy it just might work. Vim ships with a tool *
 * called 2html.vim that generates an HTML document with syntax highlighting  *
 * of the file you have open. So all we need to do is puppet Vim into running *
 * :TOhtml, parse the generated HTML and render the result using ANSI color   *
 * codes. It actually doesn't sound so bad when you say it in a nice short    *
 * sentence like that.
 *                                                                            *
 *****************************************************************************/

static int convert_to_html(const string &input, const string &output) {
    /* Construct a command line to open the file in Vim, convert it to
     * highlighted HTML, save this to the output and exit. 
     * FIXME wrap it in timeout
     * in case the user has a weird ~/.vimrc and we end up hanging.
     */
    string command = "vim -n '+set nonumber' '+TOhtml' '+w " + output +
        "' '+qa!' " + input;

    return system(command.c_str());
}

/* Decode a fragment of HTML text produced by Vim's TOhtml. It is assumed that
 * the input contains no HTML tags.
 */
string VimHighlighter::from_html(const string &text) {

    // The HTML escapes produced by TOhtml.
    struct translation_t {
        const char *key;
        char value;
    };
    static const array<translation_t, 5> HTML_DECODE { {
        { "amp;", '&' },
        { "gt;", '>' },
        { "lt;", '<' },
        { "nbsp;", ' ' },
        { "quot;", '"' },
    } };

    string result;
    for (size_t i = 0; i < text.size(); i++) {

        static const string SPAN_OPEN = "<span class=\"";
        static const string SPAN_CLOSE = "</span>";

        if (text[i] == '&' && i + 1 < text.size()) {
            // An HTML escape; let's see if we can decode it.
            bool translated = false;
            for (const translation_t &t : HTML_DECODE) {
                if (text.compare(i + 1, strlen(t.key), t.key) == 0) {
                    result += t.value;
                    i += strlen(t.key);
                    translated = true;
                    break;
                }
            }

            if (translated)
                continue;
        }

        else if (text[i] == '<') {

            // Is this a <span class="..."> ?
            if (text.compare(i, SPAN_OPEN.size(), SPAN_OPEN) == 0) {
                size_t start = i + SPAN_OPEN.size();
                size_t end = text.find("\">", start);
                if (end != string::npos) {
                    string style_name = text.substr(start, end - start);
                    auto it = m_styles.find(style_name);
                    if (it != m_styles.end()) {
                        Style s = it->second;
                        result += "\033[3" + to_string(s.fg) +
                                      ";4" + to_string(s.bg);
                        if (s.bold)
                            result += ";1";
                        if (s.underline)
                            result += ";4";
                        result += "m";
                        i += end - i + 1;
                        continue;
                    }
                }
            }
            
            // How about a </span> ?
            else if (text.compare(i, SPAN_CLOSE.size(), SPAN_CLOSE) == 0) {
                result += "\033[0m";
                i += SPAN_CLOSE.size() - 1;
                continue;
            }
        }

        result += text[i];
    }

    return result;
}

static uint8_t hex_to_int(char c) {
    assert(isxdigit(c));
    switch (c) {
        case '0' ... '9': return c - '0';
        case 'a' ... 'f': return uint8_t(c - 'a') + 10;
        case 'A' ... 'F': return uint8_t(c - 'A') + 10;
    }
    __builtin_unreachable();
}

/* Turn a six character string representing an HTML color into a value 0-7
 * representing an ANSI terminal code to most closely match that color.
 */
static unsigned to_ansi_color(const char *html_color,
        [[gnu::unused]] size_t length) {
    assert(length == 6);

    // Extract the color as RGB.
    uint8_t red = hex_to_int(html_color[0]) * 16 + hex_to_int(html_color[1]);
    uint8_t green = hex_to_int(html_color[2]) * 16 + hex_to_int(html_color[3]);
    uint8_t blue = hex_to_int(html_color[4]) * 16 + hex_to_int(html_color[5]);

    /* HTML has a 24-bit color space, but ANSI color codes have an 8-bit color
     * space. We map an HTML color onto an ANSI color by finding the "closest"
     * one using an ad hoc notion of distance between colors.
     */

    /* First, we define the ANSI colors as RGB values. These definitions match
     * what 2html uses for 8-bit color, so an HTML color intended to map
     * *exactly* to one of these should correctly end up with a distance of 0.
     */
    static const array<uint8_t[3], 8> ANSI { {
        /* black   */ { 0x00, 0x00, 0x00 },
        /* red     */ { 0xff, 0x60, 0x60 },
        /* green   */ { 0x00, 0xff, 0x00 },
        /* yellow  */ { 0xff, 0xff, 0x00 },
        /* blue    */ { 0x80, 0x80, 0xff },
        /* magenta */ { 0xff, 0x40, 0xff },
        /* cyan    */ { 0x00, 0xff, 0xff },
        /* white   */ { 0xff, 0xff, 0xff },
    } };

    // Now find the color with the least distance to the input.
    size_t min_index;
    unsigned min_distance = UINT_MAX;
    for (size_t i = 0; i < ANSI.size(); i++) {
        unsigned distance =
            (ANSI[i][0] > red ? ANSI[i][0] - red : red - ANSI[i][0]) +
            (ANSI[i][1] > green ? ANSI[i][1] - green : green - ANSI[i][1]) +
            (ANSI[i][2] > blue ? ANSI[i][2] - blue : blue - ANSI[i][2]);
        if (distance < min_distance) {
            min_index = i;
            min_distance = distance;
        }
    }

    return unsigned(min_index);
}

VimHighlighter::VimHighlighter(const string &filename) {

    // Create a temporary directory to use for scratch space.

    static const char *TMPDIR = getenv("TMPDIR");
    if (TMPDIR == nullptr) {
        TMPDIR = "/tmp";
    }

    char *temp;
    if (asprintf(&temp, "%s/tmp.XXXXXX", TMPDIR) < 0)
        return;

    if (mkdtemp(temp) == nullptr) {
        free(temp);
        return;
    }

    m_tempdir = temp;
    free(temp);

    // Convert the input to highlighted HTML.
    string output = m_tempdir + "/temp.html";
    if (convert_to_html(filename, output) != 0)
        return;

    FILE *html = fopen(output.c_str(), "r");
    if (html == nullptr)
        return;

    /* The file we have open at this point has a structure like the following:
     *
     *     <html>
     *     ...
     *     <style type="text/css">
     *     ...
     *     .MyStyle1 { color: #ff00ff ...
     *     ...
     *     </style>
     *     ...
     *     <pre id='vimCodeElement'>
     *     Hello <span class="MyStyle1">world</span>!
     *     ...
     *     </pre>
     *     ...
     *
     * Because we know its structure, we're going to discard all the advice your
     * mother told you and parse HTML and CSS using a combination of regexing
     * and string searching. Hey, I warned you this wasn't going to be pleasant.
     */

    regex_t style;
    if (regcomp(&style,
            "^\\.([[:alpha:]][[:alnum:]]+)[[:blank:]]*\\{[[:blank:]]*"
            "(color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*)?"
            "(background-color:[[:blank:]]*#([[:xdigit:]]{6});[[:blank:]]*"
             "(padding-bottom:[[:blank:]]*1px;[[:blank:]]*)?)?"
            "(font-weight:[[:blank:]]*bold;[[:blank:]]*)?"
            "(font-style:[[:blank:]]*italic;[[:blank:]]*)?"
            "(text-decoration:[[:blank:]]*underline;[[:blank:]]*)?",
            REG_EXTENDED) != 0) {
        fclose(html);
        return;
    }

    for (;;) {

        if (getline(&m_last_line, &m_last_line_sz, html) == -1) {
            fclose(html);
            regfree(&style);
            return;
        }

        if (m_last_line[0] == '.') {

            /* Setup for extraction of CSS attributes. The entries of match will
             * be:
             *   match[0]: the entire expression                             (ignore)
             *   match[1]: the name of the style
             *   match[2]: "color: #......;"                                 (ignore)
             *   match[3]:          ^^^^^^
             *   match[4]: "background-color: #......; padding-bottom: 1px;" (ignore)
             *   match[5]:                     ^^^^^^
             *   match[6]:                             ^^^^^^^^^^^^^^^^^^^^  (ignore)
             *   match[7]: "font-weight: bold;"
             *   match[8]: "font-style: italic;"                             (ignore for now)
             *   match[9]: "text-decoration: underline;"
             */
            regmatch_t match[10];
            static const size_t NMATCH = sizeof(match) / sizeof(match[0]);

            // Extract CSS attributes from this line.
            if (regexec(&style, m_last_line, NMATCH, match, 0) == REG_NOMATCH)
                continue;

            // Build a style defining these attributes.
            Style s { .fg = 9, .bg = 9, .bold = false, .underline = false };
            if (match[2].rm_so != -1)
                s.fg = to_ansi_color(&m_last_line[match[3].rm_so], 6);
            if (match[4].rm_so != -1)
                s.bg = to_ansi_color(&m_last_line[match[5].rm_so], 6);
            if (match[7].rm_so != -1)
                s.bold = true;
            if (match[9].rm_so != -1)
                s.underline = true;

            // Register this style for later highlighting.
            string name(m_last_line, match[1].rm_so,
                match[1].rm_eo - match[1].rm_so);
            m_styles[name] = s;

        } else if (strcmp(m_last_line, "<pre id='vimCodeElement'>\n") == 0) {
            // We found the content itself. We're done.
            break;
        }

    }

    regfree(&style);
    m_html = html;
}

VimHighlighter::~VimHighlighter() {
    if (m_tempdir != "") {
        (void)remove((m_tempdir + "/temp.html").c_str());
        (void)rmdir(m_tempdir.c_str());
    }
    free(m_last_line);
    if (m_html != nullptr)
        fclose(m_html);
}

string VimHighlighter::get_line(unsigned lineno) {

    if (lineno == 0)
        return "";

    while (lineno > m_lines.size() && m_html != nullptr) {
        if (getline(&m_last_line, &m_last_line_sz, m_html) == -1) {
            fclose(m_html);
            m_html = nullptr;
            break;
        }

        if (strcmp(m_last_line, "</pre>\n") == 0) {
            // End of content.
            fclose(m_html);
            m_html = nullptr;
            break;
        }

        m_lines.push_back(from_html(m_last_line));
    }

    if (lineno <= m_lines.size())
        return m_lines[lineno - 1];

    return "";
}

#ifdef TEST_VIM_HIGHLIGHTER
int main(int argc, char **argv) {
    if (argc != 2 || (argc > 1 && strcmp(argv[1], "--help") == 0)) {
        cerr << "usage: " << argv[0] << " <filename>\n";
        return EXIT_FAILURE;
    }

    VimHighlighter vh(argv[1]);

    for (unsigned i = 1; ; i++) {
        string line = vh.get_line(i);
        if (line == "")
            break;
        cout << line;
    }

    return EXIT_SUCCESS;
}
#endif

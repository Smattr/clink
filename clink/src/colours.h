/* Support for converting between different formats for describing the colour
 * and other attributes of terminal text.
 *
 * There are two different formats dealt with below:
 *
 *   1. ANSI terminal codes (e.g. "ESC[32;1m")
 *   2. Ncurses colour pairs
 */

#pragma once

#include <cstddef>
#include <string>

// equivalent of ncurses’ printw, but interpret ANSI colour codes
void printw_in_colour(const std::string &text);

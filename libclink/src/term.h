/// \file
/// \brief virtual terminal emulator
///
/// The following implements an in-memory buffer that can be operated on as if
/// it were a terminal. Data, including ANSI escape sequences, can be written to
/// the terminal through `term_send` and then the display output can be read
/// back through `term_readline`.
///
/// Only enough functionality to support the meaningful escape sequences that
/// Vim emits is implemented.

#pragma once

#include "../../common/compiler.h"
#include <stddef.h>
#include <stdio.h>

/// a virtual (in-memory) terminal
typedef struct term term_t;

/** create a new terminal
 *
 * \param t [out] A terminal handle on success
 * \param columns Width of the terminal to create
 * \param rows Height of the terminal to create
 * \returns 0 on success or an errno on failure
 */
INTERNAL int term_new(term_t **t, size_t columns, size_t rows);

/** write data to the terminal
 *
 * This function reads the given file until EOF. The read data can contain UTF-8
 * characters and/or ANSI escape sequences.
 *
 * \param t Terminal to write to
 * \param from Source to read data from
 * \returns 0 on success or an errno on failure
 */
INTERNAL int term_send(term_t *t, FILE *from);

/** read a line of data from the terminal
 *
 * \param t Terminal to read from
 * \param row 1-indexed row to read from
 * \param line [out] Read data on success
 * \returns 0 on success or an errno on failure
 */
INTERNAL int term_readline(term_t *t, size_t row, const char **line);

/** wipe any data previously rendered to this terminal
 *
 * \param t Terminal to blank
 */
void term_clear(term_t *t);

/** deallocate a terminal
 *
 * \param t Terminal to destroy
 */
INTERNAL void term_free(term_t **t);

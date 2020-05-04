#pragma once

#include <clink/db.h>

/** run Clink’s Ncurses interface
 *
 * \param db Database for searching
 * \returns 0 on success or an errno on failure
 */
int ncurses_ui(clink_db_t *db);

#pragma once

#include <clink/db.h>

/** run Clink’s TUI
 *
 * \param db Database for searching
 * \return 0 on success or an errno on failure
 */
int ui(clink_db_t *db);

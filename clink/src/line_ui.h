#pragma once

#include <clink/clink.h>

/** run Clink’s line-oriented interface
 *
 * This interface is intended to behave like Cscope’s line-oriented interface
 * for the purpose of playing nice with Vim’s Cscope support. It is not really
 * intended to be used by a human.
 *
 * \param db Database for searching
 * \returns 0 on success or an errno on failure
 */
int line_ui(clink_db_t *db);

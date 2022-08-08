#pragma once

#include <clink/clink.h>

/** build or update a Clink database
 *
 * \param db Database to operate on
 * \return 0 on success or an errno on failure
 */
int build(clink_db_t *db);

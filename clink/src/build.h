// entry point for building (or updating) a database

#pragma once

#include <clink/clink.h>

/** build or update a Clink database
 *
 * \param db Database to operate on
 * \returns 0 on success, a Clink error code on failure
 */
int build(struct clink_db *db);

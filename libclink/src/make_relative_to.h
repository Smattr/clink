#pragma once

#include "../../common/compiler.h"
#include <clink/db.h>

/** construct a path relative to the given database’s containing directory
 *
 * \param db Database whose containing directory to treat as root
 * \param path Source path to inspect
 * \return A path relative to the database’s containing directory or the
 *   original path if it could not be made relative
 */
INTERNAL const char *make_relative_to(const clink_db_t *db, const char *path);

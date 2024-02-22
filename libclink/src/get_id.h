#pragma once

#include "../../common/compiler.h"
#include <clink/db.h>

/// find the record identifier for the given path
///
/// This function returns `ENOENT` if `path` does not exist in the database’s
/// record table. The `path` parameter must be an absolute path.
///
/// \param db Database to operate on
/// \param path File path to lookup
/// \param id [out] Record identifier for this path on success
/// \return 0 on success or an errno on failure
INTERNAL int get_id(clink_db_t *db, const char *path, clink_record_id_t *id);

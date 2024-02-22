#pragma once

#include "../../common/compiler.h"
#include <clink/db.h>

/** a version of `clink_db_add_line` with a different calling convention
 *
 * This can be used to add a content line when you already know its file’s path
 * identifier, as an optimisation.
 *
 * \param db Database to operate on
 * \param path Identifier of the record for the file this line came from
 * \param lineno
 * \param lineno Line number within the file this came from
 * \param line Content of the line itself
 * \return 0 on success or a SQLite error code on failure
 */
INTERNAL int add_line(clink_db_t *db, clink_record_id_t path,
                      unsigned long lineno, const char *line);

#pragma once

#include "../../common/compiler.h"
#include "span.h"
#include <clink/db.h>
#include <clink/symbol.h>

/// a version of `clink_db_add_symbol` with a different calling convention
INTERNAL int add_symbol(clink_db_t *db, clink_category_t category, span_t name,
                        const char *path, span_t parent);

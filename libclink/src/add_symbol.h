#pragma once

#include "../../common/compiler.h"
#include "span.h"
#include <clink/db.h>
#include <clink/symbol.h>

/// variant of \p clink_symbol_t for internal use
typedef struct {
  clink_category_t category; ///< the type of this symbol
  span_t name;               ///< name of this item or referent
  const char *path;          ///< path to the containing file of this symbol
  span_t parent;             ///< optional containing definition
} symbol_t;

/// a version of `clink_db_add_symbol` with a different calling convention
INTERNAL int add_symbol(clink_db_t *db, symbol_t sym);

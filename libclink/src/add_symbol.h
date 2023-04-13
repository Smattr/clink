#pragma once

#include "../../common/compiler.h"
#include "span.h"
#include <clink/db.h>
#include <clink/symbol.h>
#include <stddef.h>

/// variant of \p clink_symbol_t for internal use
typedef struct {
  clink_category_t category; ///< the type of this symbol
  span_t name;               ///< name of this item or referent
  const char *path;          ///< path to the containing file of this symbol
  span_t parent;             ///< optional containing definition
} symbol_t;

/// a version of \p clink_db_add_symbol with a different calling convention
///
/// This function provides a way of inserting multiple symbols into the database
/// in a single operation.
///
/// If all symbols being inserted are from the same source path and the caller
/// knows the record identifier of this path, they can pass it as \p id as an
/// optimisation. If not, they can pass -1 as a default.
///
/// \param db Database to operate on
/// \param syms_size Number of elements in \p syms
/// \param syms Symbols to insert
/// \param id Identifier of the record for the source path containing all
///   symbols
/// \return 0 on success or an errno on failure
INTERNAL int add_symbols(clink_db_t *db, size_t syms_size, symbol_t *syms,
                         clink_record_id_t id);

static inline int add_symbol(clink_db_t *db, symbol_t sym) {
  return add_symbols(db, 1, &sym, -1);
}

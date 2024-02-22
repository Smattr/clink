#pragma once

#include "../../common/compiler.h"
#include "span.h"
#include <clink/db.h>
#include <clink/symbol.h>
#include <stddef.h>

/// variant of `clink_symbol_t` for internal use
typedef struct {
  clink_category_t category; ///< the type of this symbol
  span_t name;               ///< name of this item or referent
  const char *path;          ///< path to the containing file of this symbol
  span_t parent;             ///< optional containing definition
} symbol_t;

/// a version of `clink_db_add_symbol` with a different calling convention
///
/// This function provides a way of inserting multiple symbols into the database
/// in a single operation.
///
/// All symbols being inserted must be from the same source path. The caller
/// must supply the record identifier of this path as `id`.
///
/// \param db Database to operate on
/// \param syms_size Number of elements in `syms`
/// \param syms Symbols to insert
/// \param id Identifier of the record for the source path containing all
///   symbols
/// \return 0 on success or an errno on failure
INTERNAL int add_symbols(clink_db_t *db, size_t syms_size, symbol_t *syms,
                         clink_record_id_t id);

/// convenience wrapper for `add_symbols` with a single symbol
///
/// There is no way to set the `id` parameter to `add_symbols` when calling
/// through this interface. `sym.path` must be an absolute path.
///
/// \param db Database to operate on
/// \param sym Symbol to insert
/// \return 0 on success or an errno on failure
INTERNAL int add_symbol(clink_db_t *db, symbol_t sym);

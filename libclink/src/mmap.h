#pragma once

#include "../../common/compiler.h"
#include <stddef.h>

/// an mmap-ed file
typedef struct {
  void *base;  ///< start of the file’s content
  size_t size; ///< byte length of the file’s content
} mmap_t;

/** mmap a file for reading
 *
 * \param m [out] Mmap-ed range on success
 * \param filename Path to file to mmap
 * \return 0 on success or an errno on failure
 */
INTERNAL int mmap_open(mmap_t *m, const char *filename);

/** munmap a file previously mapped
 *
 * \param m Mmap-ed range to release
 */
INTERNAL void mmap_close(mmap_t m);

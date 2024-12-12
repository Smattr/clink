/// \file
/// \brief In-memory file descriptor buffering

#pragma once

#include "../../common/compiler.h"

/// a file descriptor buffer handle
///
/// This structure is intended to provide RAII semantics: creating one replaces
/// the target stream with an in-memory sink and destroying one restores the
/// original stream.
///
/// Though it is not a currently planned use case, this is intended to be
/// composable. Buffering a stream that is already being buffered should work,
/// as long as you destruct the buffers in reverse order.
typedef struct {
  int target; ///< the file descriptor being switched
  int origin; ///< a copy of the original descriptor
  char *path; ///< a temporary file where the buffered data is accrued
} fdbuf_t;

/// buffer an existing stream
///
/// \param buffer [out] A handle to the buffered stream on success
/// \param target The stream to interpose on
/// \return 0 on success or an errno on failure
INTERNAL int fdbuf_new(fdbuf_t *buffer, int target);

/// write buffered data back to the original stream
///
/// This does not unbuffer the stream. It remains interposed upon.
///
/// Conceptually, this discards data in the buffer by its actions. Writes
/// following this will accrue into the new emptied buffer.
///
/// \param header Optional data to write preceding the buffered content
/// \param buffer The buffer to write back
/// \param footer Optional data to write following the buffered content
/// \return 0 on success or an errno on failure
INTERNAL int fdbuf_writeback(const char *header, fdbuf_t *buffer,
                             const char *footer);

/// unbuffer a previously buffered stream
///
/// \param buffer A handle to the buffered stream to unbuffer
INTERNAL void fdbuf_free(fdbuf_t *buffer);

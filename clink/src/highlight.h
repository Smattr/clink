#pragma once

#include "str_queue.h"
#include <clink/clink.h>
#include <stddef.h>

/// Vim-highlight a pending series of files
///
/// This function uses multiple threads where possible.
///
/// \param db Database to insert highlighted lines into
/// \param sources Files to highlight
/// \param last_screen_row Bottom terminal row to use for status
/// \return 0 on success or an errno on failure
int highlight(clink_db_t *db, str_queue_t *sources, size_t last_screen_row);

#pragma once

#include "../../common/compiler.h"

/** create a temporary directory
 *
 * \param dir [out] Path to created directory on success
 * \returns 0 on success or an errno on failure
 */
INTERNAL int temp_dir(char **dir);

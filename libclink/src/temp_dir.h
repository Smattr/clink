#pragma once

/** create a temporary directory
 *
 * \param dir [out] Path to created directory on success
 * \returns 0 on success or an errno on failure
 */
__attribute__((visibility("internal")))
int temp_dir(char **dir);

#pragma once

/** determine a path to open as the Clink symbol database
 *
 * \param path [out] Where to store the determined path. This will be
 *   dynamically allocated and needs to be later freed by the caller.
 * \returns 0 on success, an errno on failure
 */
int get_db_path(char **path);

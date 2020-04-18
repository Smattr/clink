// utilities for dealing with file system paths

#pragma once

/** get current working directory
 *
 * \param wd [out] Path to current working directory on success
 * \returns 0 on success or an errno on failure
 */
int cwd(char **wd);

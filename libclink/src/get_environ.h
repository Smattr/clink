#pragma once

/// get a pointer to environ
__attribute__((visibility("internal")))
char **get_environ(void);

// SIGTERM handling convenience functions

#pragma once

#include <stdbool.h>

int sigterm_block(void);

bool sigterm_pending(void);

int sigterm_unblock(void);

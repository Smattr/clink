// SIGINT handling convenience functions

#pragma once

#include <stdbool.h>

int sigint_block(void);

bool sigint_pending(void);

int sigint_unblock(void);

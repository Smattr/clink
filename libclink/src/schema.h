#pragma once

#include "../../common/compiler.h"
#include <stddef.h>

INTERNAL extern const char *SCHEMA[];
INTERNAL extern const size_t SCHEMA_LENGTH;

/// get the version identifier of the database schema
INTERNAL const char *schema_version(void);

#pragma once

#include <cstddef>
#include <clink/clink.h>
#include <sys/types.h>

// build or update a Clink symbol database
int build(clink::Database &db, time_t era_start);

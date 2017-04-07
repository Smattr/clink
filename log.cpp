#include <cstdio>
#include "log.h"
#include <mutex>

using namespace std;

FILE *log_file;
mutex log_lock;

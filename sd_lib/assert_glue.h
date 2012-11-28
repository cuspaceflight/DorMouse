#include "../src/panic.h"

#ifndef NDEBUG
#define assert_param(expr) ((expr) ? (void) 0 : panic())
#endif

#ifndef NDEBUG
#include <stdio.h>
#include "memory.h"

void _debug_init();
void _debug_send(const char *message);
extern char debug_buf[512];

#define debug(...)                                                            \
    do                                                                        \
    {                                                                         \
        disable_interrupts();                                                 \
        snprintf(debug_buf, sizeof(debug_buf), __VA_ARGS__);                  \
        debug_send(debug_buf);                                                \
        debug_buf[0] = 0;                                                     \
        enable_interrupts();                                                  \
    }                                                                         \
    while (0)

#define debug_init()    _debug_init()
#define debug_send(msg) _debug_send(msg)

#else
#define debug(...)
#define debug_init()
#define debug_send(msg)
#endif

#ifndef NDEBUG
#include <stdio.h>

void debug_init();
void debug_send(const char *message);
extern char debug_buf[512];

#define debug(...)                                                            \
    do                                                                        \
    {                                                                         \
        snprintf(debug_buf, sizeof(debug_buf), __VA_ARGS__);                  \
        debug_send(debug_buf);                                                \
        debug_buf[0] = 0;                                                     \
    }                                                                         \
    while (0)

#else
#define debug_init()
#define debug_send()
#endif

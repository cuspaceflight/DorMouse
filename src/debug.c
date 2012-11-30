#include "debug.h"

#include <stdint.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "buffer.h"
#include "leds.h"
#include "memory.h"

char debug_buf[512];

void _debug_init()
{
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
              GPIO_CNF_INPUT_FLOAT, GPIO10);

    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

    usart_enable(USART1);
}

void _debug_send(const char *data)
{
    while (*data)
    {
        usart_send_blocking(USART1, *data);
        data++;
    }
}

void cm3_assert_failed()
{
    disable_interrupts();
    debug_send("Assertion failed\n");
    while(1);
}

#ifdef CM3_ASSERT_VERBOSE
void cm3_assert_failed_verbose(const char *file, int line, 
        const char *fun, const char *assert_expr)
{
    disable_interrupts();
    debug("Assertion failed:\n%s:%i:%s\n%s\n", file, line, fun, assert_expr);
    while(1);
}
#endif

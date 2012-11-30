#include "gps.h"

#include <string.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/timer.h>

#include "leds.h"

static void calculate_crc_and_ack(char *message, size_t length);
static int silence_nmea();
static int command(char *command, size_t length);

static char set_port[20 + 8] =
  /* d20 byte CFG-PRT message */
    {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00,
  /* USART 1,   Reserved, Txready,     8n1 */
     0x01,      0x00,     0x00, 0x00,  0xd0, 0x08, 0x00, 0x00,
  /* 9600 baud  ,            UBX in, UBX out (only) */
     0x80, 0x25, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01};
  /* remaining values: reserved (zero) and crc */

static char set_navmode[36 + 8] = 
 /* Sync  Sync  CFG   NAV5  36 bytes */
   {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 
 /* 0 mask  mask: dyn & fixmode */
    0x05, 0x00, 
 /* 2 airbourne 4g, auto 3d */
    0x08,           0x03};
 /* remaining values: masked, and crc */

static char expect_ack[] = 
/*   Sync  Sync  Ack   Ack   2 bytes     CLS,  ID,   CRC   CRC */
    {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

void gps_init()
{
    int ok;

    calculate_crc_and_ack(set_navmode, sizeof(set_navmode));
    cm3_assert(set_navmode[sizeof(set_navmode) - 2] == 94);
    cm3_assert(set_navmode[sizeof(set_navmode) - 1] == 235);
    cm3_assert(expect_ack[8] == 49);
    cm3_assert(expect_ack[9] == 89);

    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO2);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
              GPIO_CNF_INPUT_PULL_UPDOWN, GPIO3);

    usart_set_baudrate(USART2, 9600);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_mode(USART2, USART_MODE_TX);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

    usart_enable(USART2);

    timer_reset(TIM5);
    timer_set_mode(TIM5, TIM_CR1_CKD_CK_INT,
            TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    /* 2Hz */
    timer_set_prescaler(TIM5, 640);
    timer_set_period(TIM5, 28125);

    ok = silence_nmea();
    if (!ok) goto bail;

    ok = command(set_port, sizeof(set_port));
    if (!ok) goto bail;

bail:
    asm("nop");
}

static void calculate_crc_and_ack(char *message, size_t length)
{
    uint8_t ck_a = 0, ck_b = 0;
    size_t i;

    for (i = 2; i < length - 2; i++)
    {
        ck_a += message[i];
        ck_b += ck_a;
    }

    message[length - 2] = ck_a;
    message[length - 1] = ck_b;

    expect_ack[6] = message[3];
    expect_ack[7] = message[4];

    ck_a = 0;
    ck_b = 0;

    for (i = 2; i < 10 - 2; i++)
    {
        ck_a += expect_ack[i];
        ck_b += ck_a;
    }

    expect_ack[8] = ck_a;
    expect_ack[9] = ck_b;
}

static int silence_nmea()
{
    int attempt;
    size_t i;

    /* Configure the GPS */
    for (attempt = 0; attempt < 4; attempt++)
    {
        calculate_crc_and_ack(set_port, sizeof(silence_nmea));

        for (i = 0; i < sizeof(set_port); i++)
            usart_send_blocking(USART2, set_port[i]);

        timer_clear_flag(TIM5, TIM_SR_UIF);
        timer_set_counter(TIM5, 0);
        timer_enable_counter(TIM5);

        for (i = 0; i < sizeof(expect_ack); )
        {
            while (!timer_get_flag(TIM5, TIM_SR_UIF) &&
                   !usart_get_flag(USART2, USART_SR_RXNE));

            if (timer_get_flag(TIM5, TIM_SR_UIF))
                break;

            if (expect_ack[i] == usart_recv(USART2))
                i++;
            else
                i = 0;
        }

        timer_disable_counter(TIM5);

        if (i < sizeof(expect_ack))
            continue;
        else
            break;
    }

    while (usart_get_flag(USART2, USART_SR_RXNE))
        usart_recv(USART2);

    return attempt < 4;
}

static int command(char *command, size_t length)
{
    size_t i;

    calculate_crc_and_ack(command, length);

    for (i = 0; i < length; i++)
        usart_send_blocking(USART2, command[i]);

    timer_set_counter(TIM5, 0);
    timer_clear_flag(TIM5, TIM_SR_UIF);
    timer_enable_counter(TIM5);

    for (i = 0; i < sizeof(expect_ack); )
    {
        while (!timer_get_flag(TIM5, TIM_SR_UIF) &&
               !usart_get_flag(USART2, USART_SR_RXNE));

        if (timer_get_flag(TIM5, TIM_SR_UIF))
            break;

        if (expect_ack[i] != usart_recv(USART2))
            break;
    }

    timer_disable_counter(TIM5);

    return (i == sizeof(expect_ack));
}

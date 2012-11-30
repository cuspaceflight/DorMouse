#include "baro.h"

#include <stdint.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/f1/dma.h>

#include "general_status.h"
#include "debug.h"

/* SS PB12, MOSI PB15, MISO PB14, SCK PB13 */

static void select_slave();
static void deselect_slave();
static void read_calibration_data();
static uint8_t read_register_blocking(uint8_t reg);

/* read registers 0x00..0x03, then start a new conversion */
static char txstr[10] = { 0x80, 0x00, 0x81, 0x00, 0x82, 0x00, 0x83, 0x00,
                          0x24, 0x00 };
static char rxbuf[10];

void baro_init()
{
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO14);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13 | GPIO15);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    deselect_slave();

    spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL,
            SPI_CR1_CPHA, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_ss_output(SPI2);
    spi_enable(SPI2);

    timer_reset(TIM4);
    timer_enable_irq(TIM4, TIM_DIER_UIE);
    timer_set_mode(TIM4, TIM_CR1_CKD_CK_INT,
            TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    /* 3200 / 16 = 200 Hz */
    timer_set_prescaler(TIM4, 32);
    timer_set_period(TIM4, 5625);

    nvic_set_priority(NVIC_TIM4_IRQ, 16 * 2);
    nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, 16 * 2);
    nvic_enable_irq(NVIC_TIM4_IRQ);
    nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);

    read_calibration_data();
}

void baro_go()
{
    tim4_isr();
    timer_enable_counter(TIM4);
}

void tim4_isr()
{
    int i;

    timer_clear_flag(TIM4, TIM_SR_UIF);

    select_slave();

    for (i = 0; i < (int) sizeof(txstr); i++)
        rxbuf[i] = spi_xfer(SPI2, txstr[i]);

    deselect_slave();

    /* Data is in bytes 1, 3, 5, 7 */
    rxbuf[0] = rxbuf[1];
    rxbuf[1] = rxbuf[3];
    rxbuf[2] = rxbuf[5];
    rxbuf[3] = rxbuf[7];
    
//    debug("baro: %i %i %i %i\n", rxbuf[0], rxbuf[1], rxbuf[2], rxbuf[3]);

    /* Assume that all zeros is implausible: */
    if ((rxbuf[0] | rxbuf[1] | rxbuf[2] | rxbuf[3]) == 0)
        bad_thing_set(BARO_FAIL);
    else
        bad_thing_clear(BARO_FAIL);

}

static void read_calibration_data()
{
    int i;
    char data[8];

    for (i = 0; i < 8; i++)
        data[i] = read_register_blocking(0x04 + i);

    debug("baro calibration:");
    for (i = 0; i < 8; i++)
        debug("%02x", data[i]);
    debug("\n");

    /* TODO: do something with the calibration data */
}

static uint8_t read_register_blocking(uint8_t reg)
{
    uint8_t val;

    select_slave();
    spi_xfer(SPI2, (reg << 1) | 0x80);
    val = spi_xfer(SPI2, 0x00);
    deselect_slave();

    return val;
}

static void select_slave()
{
    gpio_clear(GPIOB, GPIO12);
}

static void deselect_slave()
{
    gpio_set(GPIOB, GPIO12);
}

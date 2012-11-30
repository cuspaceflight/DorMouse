#include "accel_lowg.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/f1/dma.h>

#include "general_status.h"
#include "buffer.h"
#include "debug.h"
#include "memory.h"

/* SS PA4, MOSI PA5, MISO PA6, SCK PA7 */

enum accel_lowg_read_state
{
    ALG_RS_NOT_STARTED = 0,
    ALG_RS_IDLE,
    ALG_RS_FIFO,
    ALG_RS_DATA,
};

static void write_register_blocking(uint8_t reg, uint8_t val);
static void select_slave();
static void deselect_slave();
static void reselect_pulse();
static void assert_idle();
static void assert_data();
static void dma_read_go();

static volatile enum accel_lowg_read_state read_state;
static char read_txstr[7] = { 0x32 | 0x80 | 0x40 };
static char read_buffer[7];
static int fifo_count;
static struct buffer_simple_push simple_push =
        { .length = 6, .sensor = ID_ACCEL_LOWG };

void accel_lowg_init()
{
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO6);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);
    deselect_slave();

    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL,
            SPI_CR1_CPHA, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_ss_output(SPI1);
    spi_enable(SPI1);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t) &SPI1_DR);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL1);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL1, DMA_CCR_PL_HIGH);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t) &SPI1_DR);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL2);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL2, DMA_CCR_PL_HIGH);

    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL1);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);

    timer_reset(TIM3);
    timer_enable_irq(TIM3, TIM_DIER_UIE);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT,
            TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    /* 3200 / 16 = 200 Hz */
    timer_set_prescaler(TIM3, 32);
    timer_set_period(TIM3, 5625);

    nvic_set_priority(NVIC_TIM3_IRQ, 16 * 6);
    nvic_set_priority(NVIC_SPI1_IRQ, 16 * 5);
    nvic_set_priority(NVIC_DMA1_CHANNEL2_IRQ, 16 * 4);
    nvic_enable_irq(NVIC_TIM3_IRQ);
    nvic_enable_irq(NVIC_SPI1_IRQ);
    nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);

    /* DATA_FORMAT: FULL_RES | D1 | D0 (16g) */
    write_register_blocking(0x31, (1 << 3) | (1 << 1) | (1 << 0));

    /* FIFO_CTL: FIFO mode (D6) */
    write_register_blocking(0x38, (1 << 6));

    /* BW_RATE: 3200Hz */
    write_register_blocking(0x2C, 0xf);
}

void accel_lowg_go()
{
    /* POWER_CTL: Measure on */
    write_register_blocking(0x2D, (1 << 3));

    read_state = ALG_RS_IDLE;
    timer_enable_counter(TIM3);
}

void tim3_isr()
{
    debug("tim3 isr. read_state = %i\n", read_state);

    timer_clear_flag(TIM3, TIM_SR_UIF);

    cm3_assert(read_state == ALG_RS_IDLE);
    assert_idle();

    /* Read register 0x39, await interrupt */
    spi_enable_rx_buffer_not_empty_interrupt(SPI1);
    select_slave();
    spi_write(SPI1, 0x39 | 0x80);
    read_state = ALG_RS_FIFO;
}

void spi1_isr()
{
    debug("spi1 isr. read_state = %i\n", read_state);
    cm3_assert(read_state == ALG_RS_FIFO);

    assert_data();
    fifo_count = (spi_read(SPI1) & ((1 << 6) - 1));
    spi_enable_rx_buffer_not_empty_interrupt(SPI1);

    debug("fifo_count = %i\n", fifo_count);

    if (fifo_count == 0)
    {
        read_state = ALG_RS_IDLE;

        bad_thing_set(ACCEL_LOWG_ACCEL_FAIL);
        deselect_slave();
    }
    else
    {
        if (fifo_count > 25)
            bad_thing_set(ACCEL_LOWG_ACCEL_FAIL);
        else
            bad_thing_clear(ACCEL_LOWG_ACCEL_FAIL);

        read_state = ALG_RS_DATA;

        reselect_pulse();
        assert_idle();

        dma_read_go();
    }
}

void dma1_channel1_isr()
{
    debug("channel 1 completed\n");
}

void dma1_channel2_isr()
{
    debug("dma1_channel2 isr. read_state = %i\n", read_state);
    cm3_assert(dma_get_interrupt_flag(DMA1, DMA_CHANNEL1, DMA_TCIF));
    assert_idle();

    dma_disable_channel(DMA1, DMA_CHANNEL1);
    dma_disable_channel(DMA1, DMA_CHANNEL2);

    buffer_simple_push(&simple_push, read_buffer + 1);
    fifo_count--;

    if (fifo_count == 0)
    {
        read_state = ALG_RS_IDLE;
        deselect_slave();
    }
    else
    {
        reselect_pulse();
        dma_read_go();
    }
}

static void dma_read_go()
{
    /* Read registers 0x32 to 0x37 */
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, 0xf);
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, 0xf);

    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t) read_txstr);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 7);
    dma_set_memory_address(DMA1, DMA_CHANNEL2, (uint32_t) read_buffer);
    dma_set_number_of_data(DMA1, DMA_CHANNEL2, 7);

    spi_enable_tx_dma(SPI1);
    spi_enable_rx_dma(SPI1);

    dma_enable_channel(DMA1, DMA_CHANNEL1);
    dma_enable_channel(DMA1, DMA_CHANNEL2);
}

static void write_register_blocking(uint8_t reg, uint8_t val)
{
    assert_idle();
    select_slave();
    spi_xfer(SPI1, reg);
    spi_xfer(SPI1, val);
    deselect_slave();
    assert_idle();
}

static void select_slave()
{
    gpio_clear(GPIOA, GPIO4);
}

static void deselect_slave()
{
    gpio_set(GPIOA, GPIO4);
}

static void reselect_pulse()
{
    int i;

    deselect_slave();

    for (i = 0; i < 20; i++)
        asm volatile("nop");

    select_slave();
}

static void assert_data()
{
    cm3_assert(SPI1_SR & SPI_SR_RXNE);
}

static void assert_idle()
{
    cm3_assert(SPI1_SR & SPI_SR_TXE);
    cm3_assert(!(SPI1_SR & SPI_SR_RXNE));
}

#include "accel_lowg.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/f1/dma.h>

#include "general_status.h"
#include "buffer.h"
#include "header.h"

/* SS PA4, MOSI PA5, MISO PA6, SCK PA7 */

enum accel_lowg_read_state
{
    ALG_RS_NONE = 0,
    ALG_RS_FIFO,
    ALG_RS_DATA,
};

static void write_register_blocking(uint8_t reg, uint8_t val);
static void select_slave();
static void deselect_slave();
static void reselect_pulse();
static void assert_idle();
static void dma_read_go();
static void push_accels();

static enum accel_lowg_read_state read_state;
static char read_txstr[7] = { 0x32 | 0x80 | 0x40 };
static char read_buffer[7];
static int fifo_count, buffer_pos;
static struct buffer_list_item *buffer;

void accel_lowg_init()
{
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);
    gpio_set(GPIOA, GPIO4);

    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL,
            SPI_CR1_CPHA, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_ss_output(SPI1);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, SPI1_DR);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL1);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL1, DMA_CCR_PL_HIGH);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL2, SPI1_DR);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL2);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL2, DMA_CCR_PL_HIGH);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);

    nvic_enable_irq(NVIC_SDIO_IRQ);
    nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);

    /* DATA_FORMAT: FULL_RES | D1 | D0 (16g) */
    write_register_blocking(0x31, (1 << 3) | (1 << 1) | (1 << 0));

    /* FIFO_CTL: FIFO mode (D6) */
    write_register_blocking(0x38, (1 << 6));
}

void accel_lowg_read()
{
    cm3_assert(read_state == ALG_RS_NONE);
    assert_idle();

    /* Read register 0x39, await interrupt */
    spi_enable_rx_buffer_not_empty_interrupt(SPI1);
    select_slave();
    spi_write(SPI1, 0x39 | 0x80);
    read_state = ALG_RS_FIFO;
}

void spi1_isr()
{
    cm3_assert(read_state == ALG_RS_FIFO);

    fifo_count = (spi_read(SPI1) & ((1 << 6) - 1));
    spi_enable_rx_buffer_not_empty_interrupt(SPI1);

    if (fifo_count == 0)
    {
        bad_thing_set(LOW_G_ACCEL_FAIL);
        deselect_slave();
    }
    else
    {
        if (fifo_count > 25)
            bad_thing_set(LOW_G_ACCEL_FAIL);
        else
            bad_thing_clear(LOW_G_ACCEL_FAIL);

        read_state = ALG_RS_DATA;

        reselect_pulse();
        assert_idle();

        dma_read_go();
    }
}

void dma1_channel2_isr()
{
    cm3_assert(dma_get_interrupt_flag(DMA1, DMA_CHANNEL1, DMA_TCIF));
    assert_idle();

    push_accels();

    if (fifo_count == 0)
    {
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

    dma_enable_channel(DMA1, DMA_CHANNEL1);
    dma_enable_channel(DMA1, DMA_CHANNEL2);
}

static void write_register_blocking(uint8_t reg, uint8_t val)
{
    select_slave();
    spi_send(SPI1, reg);
    spi_send(SPI1, val);
    deselect_slave();
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

static void assert_idle()
{
    cm3_assert(SPI1_SR & SPI_SR_TXE);
    cm3_assert(!(SPI1_SR & SPI_SR_RXNE));
}

static void push_accels()
{
    if (buffer == NULL)
    {
        buffer_alloc(&buffer);
        add_header(buffer->buf, ID_ACCEL_LOW_G);
        buffer_pos = sizeof(struct data_header);
    }

    memcpy(buffer->buf + buffer_pos, read_buffer + 1, 6);
    buffer_pos += 6;

    if (buffer_pos > sd_block_size)
    {
        buffer_queue(&buffer);
    }
}

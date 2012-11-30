#include "sdio_glue.h"

#include <stdint.h>

#include <libopencm3/cm3/assert.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/stm32/gpio.h>

/* Need to keep this separate from files that include libstm32 stuff,
 * since really bad things/conflicts happen otherwise */

void sd_hw_setup()
{
    dma_set_peripheral_address(DMA2, DMA_CHANNEL1, (uint32_t) 0x40005410);
    dma_enable_memory_increment_mode(DMA2, DMA_CHANNEL1);
    dma_set_peripheral_size(DMA2, DMA_CHANNEL1, DMA_CCR_PSIZE_32BIT);
    dma_set_memory_size(DMA2, DMA_CHANNEL1, DMA_CCR_MSIZE_32BIT);
    dma_set_priority(DMA2, DMA_CHANNEL1, DMA_CCR_PL_HIGH);

    nvic_enable_irq(NVIC_SDIO_IRQ);
}

uint8_t GPIO_ReadInputDataBit(uint32_t a, uint32_t b)
{
    cm3_assert(a == SD_DETECT_GPIO_PORT && b == SD_DETECT_PIN);
    a = a; b = b;

    if (gpio_get(GPIOC, GPIO13))
        return 0;           /* Pulled high */
    else
        return Bit_RESET;   /* Low - card present */
}

void SD_LowLevel_DeInit_ocm3(void)
{
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
           GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13);
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO2);
}

void SD_LowLevel_Init(void)
{
    /* SD_DET pull up */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO13);
    gpio_set(GPIOC, GPIO13);

    /* SD Data 0..3 CLK; CMD */
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
                    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                    GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12);
    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                    GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO2);
}

void SD_LowLevel_DMA_TxConfig(uint32_t *BufferSRC, uint32_t BufferSize)
{
    dma_disable_channel(DMA2, DMA_CHANNEL1);
    dma_clear_interrupt_flags(DMA2, DMA_CHANNEL1, 0xf);
    dma_set_read_from_memory(DMA2, DMA_CHANNEL1);
    dma_set_memory_address(DMA2, DMA_CHANNEL1, (uint32_t) BufferSRC);
    dma_set_number_of_data(DMA2, DMA_CHANNEL1, BufferSize / 4);
    dma_enable_channel(DMA2, DMA_CHANNEL1);
}

void SD_LowLevel_DMA_RxConfig(uint32_t *BufferDST, uint32_t BufferSize)
{
    dma_disable_channel(DMA2, DMA_CHANNEL1);
    dma_clear_interrupt_flags(DMA2, DMA_CHANNEL1, 0xf);
    dma_set_read_from_peripheral(DMA2, DMA_CHANNEL1);
    dma_set_memory_address(DMA2, DMA_CHANNEL1, (uint32_t) BufferDST);
    dma_set_number_of_data(DMA2, DMA_CHANNEL1, BufferSize / 4);
    dma_enable_channel(DMA2, DMA_CHANNEL1);
}

uint32_t SD_DMAEndOfTransferStatus(void)
{
    /* TODO No error checking :-( ? */
    return dma_get_interrupt_flag(DMA2, DMA_CHANNEL1, DMA_TCIF);
}

#include "accel_highg.h"

#include <stdint.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/cm3/assert.h>

#include "debug.h"

/* ADC1: PC0 == ADC123_IN10
 * ADC2: PC1 == ADC123_IN11
 * ADC3: PC2 == ADC123_IN12 */

enum adc_state
{
    ADC_NOT_STARTED = 0,
    ADC_WORKING,
    ADC_CH5_DONE,
    ADC_CH6_DONE
};

static void adc_stab_sleep();
static void dma_complete();
static void dma_restart();

static enum adc_state state;
static char buf[512];

void accel_highg_init()
{
    uint8_t channel_array[16];

    gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
            GPIO_CNF_INPUT_ANALOG, GPIO0 | GPIO1 | GPIO2);

    adc_set_dual_mode(ADC_CR1_DUALMOD_RSM);

    /* ADC1 + ADC2 dual mode */
    adc_set_dual_mode(ADC_CR1_DUALMOD_ISM);

    adc_enable_external_trigger_injected(ADC1, ADC_CR2_JEXTSEL_TIM1_TRGO);
    adc_enable_external_trigger_injected(ADC3, ADC_CR2_JEXTSEL_TIM1_TRGO);

    adc_enable_dma(ADC1);
    adc_enable_dma(ADC3);

    adc_power_on(ADC1);
    adc_power_on(ADC2);
    adc_power_on(ADC3);

    adc_stab_sleep();

    adc_reset_calibration(ADC1);
    adc_reset_calibration(ADC2);
    adc_reset_calibration(ADC3);
    adc_calibration(ADC1);
    adc_calibration(ADC2);
    adc_calibration(ADC3);

    memset(channel_array, 0, sizeof(channel_array));
    channel_array[0] = 10;
    adc_set_injected_sequence(ADC1, 1, channel_array);
    channel_array[0] = 11;
    adc_set_injected_sequence(ADC2, 1, channel_array);
    channel_array[0] = 12;
    adc_set_injected_sequence(ADC3, 1, channel_array);

    timer_reset(TIM1);
    timer_enable_irq(TIM1, TIM_DIER_UIE);
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT,
            TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    /* 500Hz */
    timer_set_prescaler(TIM1, 64);
    timer_set_period(TIM1, 2250);

    /* Generate TRGO */
    timer_set_master_mode(TIM1, TIM_CR2_MMS_UPDATE);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL5, (uint32_t) &ADC1_DR);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL5);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL5);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL5, DMA_CCR_PSIZE_32BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL5, DMA_CCR_MSIZE_32BIT);
    dma_set_priority(DMA1, DMA_CHANNEL5, DMA_CCR_PL_HIGH);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL6, (uint32_t) &ADC3_DR);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL6);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL6);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL6, DMA_CCR_PSIZE_16BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL6, DMA_CCR_MSIZE_16BIT);
    dma_set_priority(DMA1, DMA_CHANNEL6, DMA_CCR_PL_HIGH);

    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL5);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL6);
}

void accel_highg_go()
{
    timer_enable_counter(TIM1);
}

void dma1_channel5_isr()
{
    if (state == ADC_WORKING)
        state = ADC_CH5_DONE;
    else if (state == ADC_CH6_DONE)
        dma_complete();
    else
        cm3_assert(0);
}

void dma1_channel6_isr()
{
    if (state == ADC_WORKING)
        state = ADC_CH6_DONE;
    else if (state == ADC_CH5_DONE)
        dma_complete();
    else
        cm3_assert(0);
}

static void dma_complete()
{
    dma_restart();

    debug("adc stuff\n");
}

static void dma_restart()
{
    dma_set_memory_address(DMA1, DMA_CHANNEL5,
            (uint32_t) buf + 12);
    dma_set_number_of_data(DMA1, DMA_CHANNEL5, 83);
    dma_set_memory_address(DMA1, DMA_CHANNEL6,
            (uint32_t) buf + 12 + 83 * 4);
    dma_set_number_of_data(DMA1, DMA_CHANNEL6, 83);

    dma_enable_channel(DMA1, DMA_CHANNEL5);
    dma_enable_channel(DMA1, DMA_CHANNEL6);

    state = ADC_WORKING;
}

static void adc_stab_sleep()
{
    /* about 3 micro seconds */
    int i;
    for (i = 0; i < 300; i++)
        asm volatile ("nop");
}

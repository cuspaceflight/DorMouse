#include "accel_highg.h"

#include <stdint.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/f1/nvic.h>
#include <libopencm3/cm3/assert.h>

#include "debug.h"

/* ADC1: PC0 == ADC123_IN10
 * ADC2: PC1 == ADC123_IN11
 * ADC3: PC2 == ADC123_IN12 */

static void adc_stab_sleep();

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

    adc_enable_eoc_interrupt_injected(ADC1);
    adc_enable_eoc_interrupt_injected(ADC3);

    nvic_enable_irq(NVIC_ADC1_2_IRQ);
    nvic_enable_irq(NVIC_ADC3_IRQ);
}

void accel_highg_go()
{
    timer_enable_counter(TIM1);
}

void adc1_2_isr()
{
    uint32_t data = adc_read_injected(ADC1, 1);
    debug("adc1_2_isr: %08lx\n", data);
}

void adc3_isr()
{
    uint32_t data = adc_read_injected(ADC3, 1);
    debug("adc3_isr: %08lx\n", data);
}

static void adc_stab_sleep()
{
    /* about 3 micro seconds */
    int i;
    for (i = 0; i < 300; i++)
        asm volatile ("nop");
}

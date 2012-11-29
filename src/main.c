#include <libopencm3/stm32/f1/rcc.h>

#include "sd.h"
#include "buffer.h"
#include "leds.h"
#include "general_status.h"
#include "accel_lowg.h"
#include "baro.h"

/* sd: DMA2:1, SDIO, GPIOC, GPIOD
 * leds: TIM2, GPIOA, GPIOB, GPIOC
 * accel_lowg: GPIOA, DMA1:1, DMA1:2, SPI1, TIM3
 * baro: GPIOB, SPI2, TIM4, DMA1:3, DMA1:4
 * accel_highg: ADC1..3 TIM1 */

/* accel_lowg: tim3_isr, spi1_isr, dma1_channel2_isr: priority 16 * 6
 * baro: tim4_isr, dma1_channel4_isr: priority 16 * 2
 * accel_highg: dma1_channel5_isr, dma1_channel6_isr: priority 16 * 7 */

/* sd: DMA2:1 - high priority
 * accel_lowg: DMA1:1, DMA1:2 - high priority
 * baro: DMA1:3 DMA1:4 - very high priority
 * accel_highg: DMA1:5 DMA1:6 - high priority */

/* TODO: check dma error flags! */

int main()
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    /* Clock, AHB, APB2: 72MHz. APB1: 36MHz. ADC: 9MHz */

    rcc_peripheral_enable_clock(&RCC_AHBENR,
            RCC_AHBENR_SDIOEN | RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR,
            RCC_APB1ENR_TIM2EN | RCC_APB1ENR_SPI2EN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR,
            RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
            RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
            RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN |
            RCC_APB2ENR_ADC1EN);

    /* Prescale ADC 72 / 8 = 9MHz */
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV8);

    buffer_init();
    leds_init();
    general_status_init();
    accel_lowg_init();
    baro_init();

    accel_lowg_go();
    baro_go();

    /* sd_main will do sd init stuff */
    sd_main();
    return 0;
}

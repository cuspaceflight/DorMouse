#include <libopencm3/stm32/f1/rcc.h>

#include "leds.h"
#include "general_status.h"
#include "baro.h"
#include "debug.h"
#include "accel_highg.h"

#include <libopencm3/cm3/assert.h>

#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/flash.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/usart.h>

/* sd: DMA2:1, SDIO, GPIOC, GPIOD
 * leds: TIM2, GPIOA, GPIOB, GPIOC
 * accel_lowg: GPIOA, DMA1:1, DMA1:2, SPI1, TIM3
 * baro: GPIOB, SPI2, TIM4, DMA1:3, DMA1:4
 * accel_highg: ADC1..3 TIM1
 * gps: USART2, TIM5 */

/* accel_lowg: tim3_isr, spi1_isr, dma1_channel2_isr: priority 16 * 6
 * baro: tim4_isr, dma1_channel4_isr: priority 16 * 2
 * accel_highg: dma1_channel5_isr, dma1_channel6_isr: priority 16 * 7 */

/* sd: DMA2:1 - high priority
 * accel_lowg: DMA1:1, DMA1:2 - high priority
 * baro: DMA1:3 DMA1:4 - very high priority
 * accel_highg: DMA1:5 DMA1:6 - high priority
 * debug: DMA2:2 - high priority */

/* TODO: check dma error flags! */

int main()
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    /* Clock, AHB, APB2: 72MHz. APB1: 36MHz. ADC: 9MHz */

    rcc_peripheral_enable_clock(&RCC_AHBENR,
            RCC_AHBENR_SDIOEN | RCC_AHBENR_DMA1EN | RCC_AHBENR_DMA2EN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR,
            RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | 
            RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM5EN |
            RCC_APB1ENR_SPI2EN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR,
            RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
            RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
            RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN |
            RCC_APB2ENR_ADC1EN | RCC_APB2ENR_USART1EN |
            RCC_APB2ENR_TIM1EN);

    /* Prescale ADC 72 / 8 = 9MHz */
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV8);

    debug_init();
    debug_send("DorMouse\n");

    debug_send("leds_init()\n");
    leds_init();
    debug_send("general_status_init()\n");
    general_status_init();
    debug_send("baro_init()\n");
    baro_init();
    debug_send("accel_highg_init()\n");
    accel_highg_init();

    debug_send("starting\n");

    debug_send("baro_go()\n");
    baro_go();
    debug_send("accel_highg_go()\n");
    accel_highg_go();

    while(1);
}

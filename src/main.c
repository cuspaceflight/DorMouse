#include <libopencm3/stm32/f1/rcc.h>

#include "sd.h"
#include "buffer.h"

/* SD Card: DMA2:1, SDIO, GPIOC, GPIOD
 * LEDs: TIM2, GPIOA, GPIOB, GPIOC */

int main()
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_peripheral_enable_clock(&RCC_AHBENR,
            RCC_AHBENR_SDIOEN | RCC_AHBENR_DMA2EN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR,
            RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
            RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
            RCC_APB2ENR_AFIOEN);


    buffer_init();

    sd_main();
    return 0;
}

#include <libopencm3/stm32/f1/rcc.h>

#include "../sd_lib/sd.h"

/* SD Card: DMA2:1, SDIO */

int main()
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    sd_setup();
    return 1;
}

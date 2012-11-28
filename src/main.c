#include <libopencm3/stm32/f1/rcc.h>

#include "sd.h"
#include "sd_buffer.h"

/* SD Card: DMA2:1, SDIO */

int main()
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    sd_buffer_init();

    sd_main();
    return 0;
}

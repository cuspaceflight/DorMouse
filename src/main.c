#include <libopencm3/stm32/f1/rcc.h>

#include "sd.h"
#include "buffer.h"

/* SD Card: DMA2:1, SDIO */

int main()
{
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    buffer_init();

    sd_main();
    return 0;
}

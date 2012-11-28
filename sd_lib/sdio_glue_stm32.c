#include "sdio_glue.h"

#include "stm32f10x_sdio.h"
#include "stm32_eval_sdio_sd.h"

void sd_setup()
{
    sd_setup_ocm3();
    SD_Init();
}

void SD_LowLevel_DeInit(void)
{
    SDIO_ClockCmd(DISABLE);
    SDIO_SetPowerState(SDIO_PowerState_OFF);
    SDIO_DeInit();
    SD_LowLevel_DeInit_ocm3();
}

void sdio_irq(void)
{
    SD_ProcessIRQSrc();
}

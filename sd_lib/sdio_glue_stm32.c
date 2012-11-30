#include "sdio_glue.h"

#include "stm32f10x_sdio.h"
#include "stm32_eval_sdio_sd.h"

#include "../src/sd.h"
#include "../src/debug.h"

/* stm32_eval_sdio_sd.c whyyy :-( */
extern SD_CardInfo SDCardInfo;

uint32_t sd_card_capacity;

int sd_init()
{
    SD_Error status = 0;

    debug_send("SD_Init()\n");
    status = SD_Init();
    if (status != SD_OK)
        return status;

    debug_send("sd_card_capacity get\n");
    sd_card_capacity = SDCardInfo.CardCapacity;

    if (sd_card_capacity & (sd_block_size - 1))
        return SD_ADDR_MISALIGNED;
    if (SDCardInfo.CardBlockSize != sd_block_size)
        return SD_BLOCK_LEN_ERR;
    if (sd_card_capacity < sd_block_size)
        return SD_ERROR;

    return status;
}

void sd_reset()
{
    SD_DeInit();
}

int sd_read(uint32_t address, char *buf, uint16_t size)
{
    SD_Error status = SD_OK;
    SDTransferState transfer_state = SD_TRANSFER_BUSY;

    debug("SD_ReadBlock()\n");
    status = SD_ReadBlock((uint8_t *) buf, address, size);
    if (status != SD_OK)
        goto bail;

    debug("SD_WaitReadOperation()\n");
    status = SD_WaitReadOperation();
    if (status != SD_OK)
        goto bail;

    debug("wait for SD_GetStatus() == SD_TRANSFER_OK\n");
    while (transfer_state != SD_TRANSFER_OK)
        transfer_state = SD_GetStatus();

    if (transfer_state == SD_TRANSFER_ERROR)
    {
        status = SD_ERROR;
        goto bail;
    }

bail:
    return status;
}

int sd_write(uint32_t address, char *buf, uint16_t size)
{
    SD_Error status = SD_OK;
    SDTransferState transfer_state = SD_TRANSFER_BUSY;

    status = SD_WriteBlock((uint8_t *) buf, address, size);
    if (status != SD_OK)
        goto bail;

    status = SD_WaitWriteOperation();
    if (status != SD_OK)
        goto bail;

    while (transfer_state != SD_TRANSFER_OK)
        transfer_state = SD_GetStatus();

    if (transfer_state == SD_TRANSFER_ERROR)
    {
        status = SD_ERROR;
        goto bail;
    }

bail:
    return status;
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

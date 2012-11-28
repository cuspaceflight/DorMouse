#include <stdint.h>

/* In order to avoid conflicts between libopencm3 and libstm32, glue bits
 * are split across two files. This is super super ugly. Massive TODO:
 * write a decent SDIO driver for libopencm3. */

/* src -> glue */
#include "sd_lib.h"

/* SDIO Driver -> STM32 glue */
void SD_LowLevel_DeInit(void);

/* SDIO Driver -> OCM3 glue */

/*  Emulate the libstm32 GPIO module that we arn't using *
 *  Only called by SD_Detect. Return Bit_RESET if SD card present */
#define SD_DETECT_PIN          0xDEAD
#define SD_DETECT_GPIO_PORT    0xBEEF
#define Bit_RESET              0x11
uint8_t GPIO_ReadInputDataBit(uint32_t a, uint32_t b);

/*  Used to configure the SDIO peripheral */
#define SDIO_INIT_CLK_DIV      178 /* 72/(178+2) = 0.4MHz (the maximum) */
#define SDIO_TRANSFER_CLK_DIV  0   /* max speed: 72/(0+2) = 18 MHz */

/*  Call setup and teardown routines */
void SD_LowLevel_Init(void);
void SD_LowLevel_DMA_TxConfig(uint32_t *BufferSRC, uint32_t BufferSize);
void SD_LowLevel_DMA_RxConfig(uint32_t *BufferDST, uint32_t BufferSize);
uint32_t SD_DMAEndOfTransferStatus(void);

/* STM32 glue -> OCM3 glue */
void SD_LowLevel_DeInit_ocm3(void);


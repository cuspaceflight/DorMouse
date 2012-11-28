/* These are the functions, defined in one of the glue files, that are
 * exported to the rest of src/. See sdio_glue.h for :-( */

extern uint32_t sd_card_capacity;

void sd_hw_setup();
int sd_init();
void sd_reset();
int sd_read(uint32_t address, char *buf, uint16_t size);
int sd_write(uint32_t address, char *buf, uint16_t size);

#include "sd.h"

#include <stdint.h>
#include "../sd_lib/sd_lib.h"
#include "buffer.h"
#include "leds.h"

/*
 * LEDS:
 *  green/green:  OK, IDLE
 *  green/orange: OK, BUSY
 *  red/green:    ERR, IDLE
 *  red/orange:   ERR, BUSY
 */

static void fail_backoff(int status);
static int find_unused_block(uint32_t *first_address);
static int is_block_used(uint32_t address, int *is_used);

enum
{
    /* 1 through 40 are used by ../sd_lib */
    SD_NO_UNUSED_BLOCK = 101
};

void sd_main()
{
    int status = 0;
    uint32_t address = 0;
    struct buffer_list_item *item;
    enum led_colour colour_a = LED_GREEN;

    sd_hw_setup();

    /* rawr, I'm a velociraptor */
reset:
    leds_set(LED_SD, colour_a, LED_ORANGE);

    status = sd_init();
    if (status != 0) goto bail;

    status = find_unused_block(&address);
    if (status != 0) goto bail;

    leds_set(LED_SD, colour_a, LED_GREEN);

    while (1)
    {
        buffer_pop(&item);

        leds_set(LED_SD, colour_a, LED_ORANGE);
        status = sd_write(address, item->buf, sd_block_size);
        leds_set(LED_SD, colour_a, LED_GREEN);

        if (status != 0)
        {
            buffer_unpop(&item);
            goto bail;
        }
        else
        {
            buffer_free(&item);
            address += sd_block_size;
        }
    }

bail:
    colour_a = LED_RED;
    sd_reset();
    fail_backoff(status);
    goto reset;
}

static int find_unused_block(uint32_t *first_address)
{
    uint32_t lower, upper, midpoint;
    int status, used;

    /* Inclusive */
    used = 1;
    lower = 0;
    upper = (sd_card_capacity / sd_block_size) - 1;

    while (lower < upper)
    {
        /* Division by block size => no overflow */
        midpoint = (lower + upper) / 2;

        status = is_block_used(midpoint * sd_block_size, &used);
        if (status != 0) return status;

        if (used)
            lower = midpoint + 1;
        else
            upper = midpoint;
    }

    if (used || lower != upper)
    {
        /* Couldn't find an unused block */
        return SD_NO_UNUSED_BLOCK;
    }
    else
    {
        *first_address = lower;
        return 0;
    }
}

static int is_block_used(uint32_t address, int *is_used)
{
    static char buf[sd_block_size];

    int status;

    /* Is this block used? First header byte will be nonzero */
    status = sd_read(address, buf, sd_block_size);
    if (status != 0) return status;

    *is_used = (buf[0] != 0);
    return 0;
}

static void fail_backoff(int status)
{
    status = status;
    /* TODO */
}


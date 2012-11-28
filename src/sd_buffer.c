#include "sd_buffer.h"

#include <stdlib.h>
#include "sd.h"

#define block_count 100

/* Linked list item */
struct list_item
{
    char *buf;
    struct list_item *next;
};

static char blocks[sd_block_size * block_count];
static struct list_item block_list_items[block_count];

static struct list_item *list_free, *list_queue;

#define disable_interrupts() asm("cpsid i")
#define enable_interrupts()  asm("cpsie i")

/* We need to take care wrt. interrupts and memory barriers in this file. */

/* At boot, hopefully before any interrupts are enabled. */
void sd_buffer_init()
{
    int i;

    disable_interrupts();

    list_free = &block_list_items[0];

    for (i = 0; i < block_count; i++)
    {
        block_list_items[i].buf = &blocks[sd_block_size * i];
        block_list_items[i].next = &block_list_items[i - 1];
    }

    block_list_items[block_count - 1].next = NULL;

    enable_interrupts();
}

void sd_buffer_out_get(char **buf)
{
    /* Make gcc happy; TODO */
    buf = buf;
    list_queue = list_queue;
}

void sd_buffer_out_done()
{

}

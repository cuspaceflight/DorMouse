#include "sd.h"

struct buffer_list_item
{
    char buf[sd_block_size];
    struct buffer_list_item *next;
};

/*
 * To add data to the buffer:
 *  - buffer_alloc removes an item from the free list, and returns it
 *  - write data into item->buf
 *  - buffer_queue adds that item to the queue list; i.e., the 
 *    'to be written queue'
 * If you change your mind, buffer_free can be used straight away after
 * _alloc.
 *
 * To get data from the buffer to be written to the card
 *  - buffer_pop pops an item off the queue
 *  - write item->buf to card
 *  - buffer_free returns that item to the free list
 * If write fails, instead of buffer_free, use buffer_unpop
 */

void buffer_init();
void buffer_alloc(struct buffer_list_item **item);
void buffer_queue(struct buffer_list_item **item);
void buffer_pop(struct buffer_list_item **item);
void buffer_free(struct buffer_list_item **item);
void buffer_unpop(struct buffer_list_item **item);

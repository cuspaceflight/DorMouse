#include "sd.h"

#include <string.h>

#include "header.h"

struct buffer_list_item
{
    char buf[sd_block_size];
    struct buffer_list_item *next;
};

struct buffer_simple_push
{
    struct buffer_list_item *buffer;
    int pos;
    size_t length;
    enum sensor_id sensor;
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
 * Simple push is for fixed length blobs of data: allocate a struct
 * buffer_simple_push and set ->length; then call buffer_simple_push lots.
 *
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

void buffer_simple_push(struct buffer_simple_push *settings, const char *data);

#include "buffer.h"

#include <stdlib.h>
#include <assert.h>
#include "sd.h"

#define block_count 100

struct buffer_list
{
    struct buffer_list_item *head;
    struct buffer_list_item *tail;

    /* stats only */
    int length_max;
    int length_min;
    int length;
};

static struct buffer_list_item items[block_count];
static struct buffer_list items_free, items_queue;
static int overflow;

/* We need to take care wrt. interrupts and memory barriers in this file. */
#define disable_interrupts() asm volatile ("cpsid i")
#define enable_interrupts()  asm volatile ("cpsie i")
#define wait_for_interrupt() asm volatile ("wfi")
#define memory_barrier()     asm volatile ("dmb" ::: "memory")

static void list_pop_head(struct buffer_list *list,
        struct buffer_list_item **item);
static void list_push_head(struct buffer_list *list,
        struct buffer_list_item **item);
static void list_push_tail(struct buffer_list *list,
        struct buffer_list_item **item);
static void list_length_stats_reset(struct buffer_list *list);
static void list_length_stats_update(struct buffer_list *list);

/* At boot, hopefully before any interrupts are enabled. */
void buffer_init()
{
    int i;

    disable_interrupts();
    memory_barrier();

    items_free.head = &items[0];

    for (i = 0; i < block_count - 1; i++)
        items[i].next = &items[i + 1];

    /* leaving items[block_count - 1].next = NULL; */

    items_free.tail = &items[block_count - 1];
    items_free.length = block_count;
    list_length_stats_reset(&items_free);

    memory_barrier();
    enable_interrupts();
}

void buffer_alloc(struct buffer_list_item **item)
{
    disable_interrupts();
    memory_barrier();

    if (items_free.head != NULL)
    {
        list_pop_head(&items_free, item);
    }
    else
    {
        list_pop_head(&items_queue, item);
        overflow = 1;
    }

    memory_barrier();
    enable_interrupts();
}

void buffer_queue(struct buffer_list_item **item)
{
    disable_interrupts();
    memory_barrier();

    list_push_tail(&items_queue, item);
    *item = NULL;

    memory_barrier();
    enable_interrupts();
}

void buffer_pop(struct buffer_list_item **item)
{
    disable_interrupts();
    memory_barrier();

    while (items_queue.head == NULL)
    {
        wait_for_interrupt();
        enable_interrupts();

        /* Allow interrupts to execute... */

        disable_interrupts();
        memory_barrier();
    }

    list_pop_head(&items_queue, item);

    memory_barrier();
    enable_interrupts();
}

void buffer_free(struct buffer_list_item **item)
{
    disable_interrupts();
    memory_barrier();
    list_push_tail(&items_free, item);
    memory_barrier();
    enable_interrupts();
}

void buffer_unpop(struct buffer_list_item **item)
{
    disable_interrupts();
    memory_barrier();
    list_push_head(&items_free, item);
    memory_barrier();
    enable_interrupts();
}

static void list_pop_head(struct buffer_list *list,
        struct buffer_list_item **item)
{
    *item = list->head;
    list->head = list->head->next;
    (*item)->next = NULL;

    /* Handle removing the last item */
    if (list->head == NULL)
    {
        assert(list->tail == *item);
        assert(list->length == 1);
        list->tail = NULL;
    }

    list->length--;
    list_length_stats_update(list);
}

static void list_push_head(struct buffer_list *list,
        struct buffer_list_item **item)
{
    assert((*item)->next == NULL);

    (*item)->next = list->head;
    list->head = *item;

    *item = NULL;

    list->length++;
    list_length_stats_update(list);
}

static void list_push_tail(struct buffer_list *list,
        struct buffer_list_item **item)
{
    assert((*item)->next == NULL);

    list->tail->next = *item;
    list->tail = *item;

    *item = NULL;

    list->length++;
    list_length_stats_update(list);
}

static void list_length_stats_reset(struct buffer_list *list)
{
    list->length_min = list->length_max = list->length;
}

static void list_length_stats_update(struct buffer_list *list)
{
    if (list->length < list->length_min)
        list->length_min = list->length;
    if (list->length > list->length_max)
        list->length_max = list->length;
}
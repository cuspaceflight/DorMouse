#include "header.h"

#include <string.h>

void add_header(char *buf, enum sensor_id sensor_id)
{
    struct data_header *hdr = (struct data_header *) buf;
    memset(hdr, 0, sizeof(struct data_header));
    hdr->nonzero_value = 0xFF;
    hdr->sensor_id = sensor_id;
    /* TODO hdr->clock */
}

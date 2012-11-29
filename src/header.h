#include <stdint.h>

#ifndef DORMOUSE_HEADER_H
#define DORMOUSE_HEADER_H

enum sensor_id
{
    ID_ACCEL_LOWG,
    ID_ACCEL_HIGHG,
    ID_GPS,
    ID_BARO,
    ID_MISC
};

struct data_header
{
    uint8_t nonzero_value;
    enum sensor_id sensor_id;
    uint32_t clock;
};

void add_header(char *buf, enum sensor_id sensor_id);

#endif /* DORMOUSE_HEADER_H */

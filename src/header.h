#include <stdint.h>

enum sensor_id
{
    ID_ACCEL_LOW_G,
    ID_ACCEL_HIGH_G,
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

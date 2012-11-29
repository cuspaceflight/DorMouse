enum bad_thing
{
    NO_BAD_THINGS = 0,

    BAROMETER_FAIL = 1,
    HIGH_G_ACCEL_FAIL = 2,
    LOW_G_ACCEL_FAIL = 3,
    BUFFER_HALF_FULL = 4,
    BUFFER_OVERFLOW = 5,

    BAD_THING_COUNT = 6
};

void general_status_init();
void bad_thing_set(enum bad_thing what);
void bad_thing_clear(enum bad_thing what);

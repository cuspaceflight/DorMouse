#include "general_status.h"

#include "leds.h"

static int bad_things_now, bad_things_all;
static enum led_colour thing_to_colour(enum bad_thing thing);
static enum bad_thing worst_bad_thing(int things);
static void update_general_led();

void general_status_init()
{
    update_general_led();
}

void bad_thing_set(enum bad_thing what)
{
    bad_things_now |= (1 << what);
    bad_things_all |= (1 << what);
    update_general_led();
}

void bad_thing_clear(enum bad_thing what)
{
    bad_things_now &= ~(1 << what);
    update_general_led();
}

static void update_general_led()
{
    leds_set(LED_GENERAL,
            thing_to_colour(worst_bad_thing(bad_things_now)),
            thing_to_colour(worst_bad_thing(bad_things_all)));
}

static enum led_colour thing_to_colour(enum bad_thing thing)
{
    if (thing == BUFFER_OVERFLOW)
        return LED_RED;
    else if (thing == NO_BAD_THINGS)
        return LED_GREEN;
    else
        return LED_ORANGE;
}

static enum bad_thing worst_bad_thing(int things)
{
    enum bad_thing i;

    for (i = BAD_THING_COUNT - 1; i >= NO_BAD_THINGS + 1; i--)
        if (things & (1 << i))
            return i;

    return NO_BAD_THINGS;
}

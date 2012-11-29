
enum led_name
{
    LED_GENERAL   = 0,
    LED_SD        = 1,
    LED_GPS       = 2,
    LED_COUNT     = 3
};

enum led_colour
{
    LED_OFF    = 0x0,
    LED_GREEN  = 0x1,
    LED_RED    = 0x2,
    LED_ORANGE = 0x3
};

void leds_setup();
void leds_set(enum led_name which, enum led_colour a, enum led_colour b);

#include "leds.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f1/nvic.h>

#include "memory.h"

/*
 * LED     GRN RED
 * GENERAL PC7 PA8
 * GPS     PC3 PC4
 * SD      PC5 PC6
 * 
 * CHRG LED  PA1
 * !COMPLETE PA0
 * CHARGING  PB9
 */

static enum led_colour states[LED_COUNT * 2];
static int toggle;

static void set_two_colour_leds();
static void set_charging_led();

void leds_init()
{
    /* !COMPLETE pulled up */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO0);
    gpio_set(GPIOA, GPIO0);

    /* CHARGING pulled down */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO9);

    /* LEDS */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
            GPIO_CNF_OUTPUT_PUSHPULL, GPIO1 | GPIO8);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
            GPIO_CNF_OUTPUT_PUSHPULL, GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO7);

    /* Timer */
    timer_reset(TIM2);

    nvic_enable_irq(NVIC_TIM2_IRQ);
    timer_enable_irq(TIM2, TIM_DIER_UIE);

    /* 2Hz */
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
            TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM2, 1280);
    timer_set_period(TIM2, 28125);

    timer_enable_counter(TIM2);
}

void leds_set(enum led_name which, enum led_colour a, enum led_colour b)
{
    states[which] = a;
    states[which + LED_COUNT] = b;
    memory_barrier();
}

void tim2_isr()
{
    timer_clear_flag(TIM2, TIM_SR_UIF);

    set_two_colour_leds();
    set_charging_led();

    toggle = !toggle;
}

static void set_two_colour_leds()
{
    uint32_t val, mask;
    enum led_colour *state;
   
    state = (toggle ? states : states + LED_COUNT);

    val = state[LED_GPS] << 3;
    mask = LED_ORANGE << 3;
    gpio_set(GPIOC, val & mask);
    gpio_clear(GPIOC, (~val) & mask);

    val = state[LED_SD] << 5;
    mask = LED_ORANGE << 5;
    gpio_set(GPIOC, val & mask);
    gpio_clear(GPIOC, (~val) & mask);

    if (state[LED_GENERAL] & LED_GREEN)
        gpio_set(GPIOC, GPIO7);
    else
        gpio_clear(GPIOC, GPIO7);

    if (state[LED_GENERAL] & LED_RED)
        gpio_set(GPIOA, GPIO8);
    else
        gpio_clear(GPIOA, GPIO8);
}

static void set_charging_led()
{
    int charging = gpio_get(GPIOB, GPIO9);
    int complete = !gpio_get(GPIOA, GPIO0);

    /*
     *            !charging charging 
     * !complete   off       solid
     * complete    n/a       flash
     */

    if (charging && (!complete || toggle))
        gpio_set(GPIOA, GPIO1);
    else
        gpio_clear(GPIOA, GPIO1);
}

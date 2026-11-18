/**
 * TrackbotPico - Raspberry Pi Pico W encoder coprocessor for TrackBot.
 *
 * Three parts that never block each other:
 *   1. PIO hardware counts encoder edges            (encoder.c)
 *   2. A 1 kHz timer turns counts into a live state  (sampler.c)
 *   3. An I2C IRQ serves the register image to the Pi (i2c_target.c, registers.c)
 *
 * The main loop only does housekeeping: feeds the watchdog, executes COMMAND
 * writes posted by the I2C IRQ, blinks the LED, and prints debug status.
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"

#include "config.h"
#include "registers.h"
#include "encoder.h"
#include "sampler.h"
#include "i2c_target.h"
#include "commands.h"

// ---- Startup ----------------------------------------------------------------

#define MORSE_UNIT_MS   60     // one dot; gaps follow standard Morse ratios (20 wpm = 60 ms, 30 wpm = 40 ms)

// Flash a run of dots on the LED: on 1 unit, off 1 unit between dots.
static void morse_dots(int count) {
    for (int i = 0; i < count; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        sleep_ms(MORSE_UNIT_MS);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        sleep_ms(MORSE_UNIT_MS);
    }
}

// Blink "hi" in Morse (H = 4 dots, I = 2 dots) as a startup greeting.
// Blocks for 20 units (0.8 s at 40 ms), so call it before the watchdog is armed.
static void morse_hi(void) {
    morse_dots(4);                      // H
    sleep_ms(2 * MORSE_UNIT_MS);        // letter gap = 3 units (1 already elapsed)
    morse_dots(2);                      // I
    sleep_ms(6 * MORSE_UNIT_MS);        // word gap before the status blink starts
}

// ---- Main-loop helpers ------------------------------------------------------

// True if either track's velocity exceeds MOTION_THRESHOLD_CPS in either direction.
static bool motors_moving(void) {
    encoder_state_t s;
    sampler_get_live(&s);
    return s.left_vel  >  MOTION_THRESHOLD_CPS || s.left_vel  < -MOTION_THRESHOLD_CPS ||
           s.right_vel >  MOTION_THRESHOLD_CPS || s.right_vel < -MOTION_THRESHOLD_CPS;
}

// A Morse letter as the on-time of each element in units (dot = 1, dash = 3).
typedef struct {
    uint8_t count;
    uint8_t units[4];
} morse_letter_t;

static const morse_letter_t MORSE_S = { 3, { 1, 1, 1 } };  // static:  . . .
static const morse_letter_t MORSE_M = { 2, { 3, 3 } };     // moving:  - -

// Status blink, once per STATUS_BLINK_PERIOD_MS: S when the motors are
// static, M when they are moving. Non-blocking state machine: each call
// switches the LED only when the current element or gap has run its time.
static void status_led_service(absolute_time_t now) {
    static absolute_time_t period_start;
    static absolute_time_t next_change;
    static const morse_letter_t *letter;    // NULL = dark until next period
    static uint8_t index;
    static bool led_on;

    if (letter) {
        if (absolute_time_diff_us(next_change, now) < 0) {
            return;
        }
        if (led_on) {                       // element done -> gap or finish
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
            led_on = false;
            if (++index >= letter->count) {
                letter = NULL;
                return;
            }
            next_change = delayed_by_ms(now, MORSE_UNIT_MS);
        } else {                            // gap done -> next element
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
            led_on = true;
            next_change = delayed_by_ms(now, letter->units[index] * MORSE_UNIT_MS);
        }
        return;
    }

    if (absolute_time_diff_us(period_start, now) < 0) {
        return;
    }
    period_start = delayed_by_ms(now, STATUS_BLINK_PERIOD_MS);
    letter = motors_moving() ? &MORSE_M : &MORSE_S;
    index = 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    led_on = true;
    next_change = delayed_by_ms(now, letter->units[0] * MORSE_UNIT_MS);
}

// Print counts, velocities and STATUS over USB serial every DEBUG_PRINT_MS.
static void debug_print_service(absolute_time_t now) {
#if DEBUG_PRINT_MS > 0
    static absolute_time_t next;
    if (absolute_time_diff_us(next, now) < 0) {
        return;
    }
    next = delayed_by_ms(now, DEBUG_PRINT_MS);

    encoder_state_t s;
    sampler_get_live(&s);
    printf("t=%lu L=%ld (%ld/s) R=%ld (%ld/s) status=0x%02X\n",
           (unsigned long)s.timestamp_us,
           (long)s.left_count, (long)s.left_vel,
           (long)s.right_count, (long)s.right_vel,
           registers_get_status());
#else
    (void)now;
#endif
}

// ---- Entry point ------------------------------------------------------------

// Bring up stdio, the LED chip, registers, encoders, sampler and I2C target,
// then arm the watchdog and run the housekeeping loop forever.
int main(void) {
    // Check before anything else touches the watchdog: true only for a real
    // watchdog timeout, not for watchdog_reboot()/reset_usb_boot().
    bool wdt_restart = watchdog_enable_caused_reboot();

    stdio_init_all();

    // The Pico W LED hangs off the CYW43 chip. arch_none brings up the chip
    // for GPIO only; Wi-Fi is never started.
    if (cyw43_arch_init()) {
        printf("cyw43 init failed\n");
        return -1;
    }

    printf("TrackbotPico fw 0x%02X starting%s\n", FW_VERSION,
           wdt_restart ? " (after watchdog reset)" : "");

    // Order matters: the register image and sampler must be ready before the
    // I2C target can answer a snapshot read.
    registers_init(wdt_restart);
    encoder_init();
    if (!sampler_init()) {
        printf("sampler timer init failed\n");
        return -1;
    }
    i2c_target_init();

    // Greeting runs while the sampler and I2C (both interrupt-driven) are
    // already live; COMMAND writes wait until the main loop starts.
    morse_hi();

    // Pause on debug so breakpoints don't trigger a reset.
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);

    while (true) {
        absolute_time_t now = get_absolute_time();

        watchdog_update();
        commands_service();
        status_led_service(now);
        debug_print_service(now);

        tight_loop_contents();
    }
}

/**
 * registers.c - Register image behind the I2C target.
 *
 * Reads are served straight from `image`, which is always pre-built, so the
 * IRQ never has to compute anything and never clock-stretches.
 */
#include <string.h>

#include "pico/sync.h"

#include "config.h"
#include "registers.h"
#include "sampler.h"
#include "commands.h"

static uint8_t  image[REG_MAP_SIZE];
static uint8_t  reg_ptr;
static uint16_t seq;

// Copy the sampler's live state into the frozen snapshot block. Runs in the
// I2C IRQ when the Pi sets the pointer to REG_SNAPSHOT.
static void latch_snapshot(void) {
    encoder_state_t live;
    sampler_get_live(&live);

    snapshot_t snap = {
        .timestamp_us = live.timestamp_us,
        .left_count   = live.left_count,
        .right_count  = live.right_count,
        .left_vel     = live.left_vel,
        .right_vel    = live.right_vel,
        .seq          = ++seq,
    };
    memcpy(&image[REG_SNAPSHOT], &snap, sizeof snap);
}

// Reset the image to power-up defaults: identity bytes, default velocity
// window, and STATUS_WDT_RESTART if the last reset was the watchdog.
void registers_init(bool watchdog_restart) {
    memset(image, 0, sizeof image);
    image[REG_WHO_AM_I]      = WHO_AM_I_VALUE;
    image[REG_FW_VERSION]    = FW_VERSION;
    image[REG_CONFIG]        = 0x00;
    image[REG_VEL_WINDOW_MS] = VEL_WINDOW_DEFAULT_MS;
    if (watchdog_restart) {
        image[REG_STATUS] |= STATUS_WDT_RESTART;
    }
    reg_ptr = 0;
    seq = 0;
}

// Point at a register. Pointing at REG_SNAPSHOT freezes a fresh snapshot so
// the following block read is consistent.
void registers_set_pointer(uint8_t reg) {
    reg_ptr = reg;
    if (reg == REG_SNAPSHOT) {
        latch_snapshot();
    }
}

// Serve one read byte and auto-increment. Side effects: PING counts up, and
// reading STATUS clears STATUS_ZEROED.
uint8_t registers_read_next(void) {
    uint8_t reg = reg_ptr++;
    if (reg >= REG_MAP_SIZE) {
        return 0xFF;
    }
    if (reg == REG_PING) {
        return ++image[REG_PING];   // first ping returns 1
    }
    uint8_t value = image[reg];
    if (reg == REG_STATUS) {
        image[REG_STATUS] &= (uint8_t)~STATUS_ZEROED;   // "since last read"
    }
    return value;
}

// Apply one written byte and auto-increment: COMMAND is queued for the main
// loop, CONFIG is masked, VEL_WINDOW_MS is clamped; read-only regs ignore it.
void registers_write_next(uint8_t value) {
    uint8_t reg = reg_ptr++;
    switch (reg) {
    case REG_COMMAND:
        commands_post(value);   // executed later by the main loop
        break;
    case REG_CONFIG:
        image[REG_CONFIG] = value & CONFIG_WRITABLE_MASK;
        break;
    case REG_VEL_WINDOW_MS:
        if (value < VEL_WINDOW_MIN_MS) value = VEL_WINDOW_MIN_MS;
        if (value > VEL_WINDOW_MAX_MS) value = VEL_WINDOW_MAX_MS;
        image[REG_VEL_WINDOW_MS] = value;
        break;
    default:
        break;                  // read-only or unmapped: ignore
    }
}

// Plain accessors for the sampler and main loop (single-byte reads are atomic).
uint8_t registers_get_config(void)        { return image[REG_CONFIG]; }
uint8_t registers_get_vel_window_ms(void) { return image[REG_VEL_WINDOW_MS]; }
uint8_t registers_get_status(void)        { return image[REG_STATUS]; }

// Set STATUS bits. STATUS is touched from the main loop and from IRQs, so the
// read-modify-write runs with interrupts off.
void registers_set_status(uint8_t bits) {
    uint32_t irq = save_and_disable_interrupts();
    image[REG_STATUS] |= bits;
    restore_interrupts(irq);
}

// Clear STATUS bits, with interrupts off for the same reason.
void registers_clear_status(uint8_t bits) {
    uint32_t irq = save_and_disable_interrupts();
    image[REG_STATUS] &= (uint8_t)~bits;
    restore_interrupts(irq);
}

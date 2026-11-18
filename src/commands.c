/**
 * commands.c - Execute COMMAND register writes from the main loop.
 */
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "pico/bootrom.h"

#include "registers.h"
#include "sampler.h"
#include "commands.h"

// Single-slot mailbox; -1 = empty. A second command arriving before the main
// loop services the first replaces it. TODO: small queue if that matters.
static volatile int16_t pending_cmd = -1;

// Store a command byte for the main loop (called from the I2C IRQ).
void commands_post(uint8_t cmd) {
    pending_cmd = cmd;
}

// Take the pending command and empty the mailbox in one step; -1 if none.
static int16_t take_pending(void) {
    uint32_t irq = save_and_disable_interrupts();
    int16_t cmd = pending_cmd;
    pending_cmd = -1;
    restore_interrupts(irq);
    return cmd;
}

// Run the pending command, if any: zero counters, clear STATUS flags, or
// reboot into the USB bootloader.
void commands_service(void) {
    int16_t cmd = take_pending();
    if (cmd < 0) {
        return;
    }

    switch ((uint8_t)cmd) {
    case CMD_ZERO_BOTH:
        sampler_request_zero(true, true);
        break;
    case CMD_ZERO_LEFT:
        sampler_request_zero(true, false);
        break;
    case CMD_ZERO_RIGHT:
        sampler_request_zero(false, true);
        break;
    case CMD_CLEAR_STATUS:
        registers_clear_status(STATUS_WDT_RESTART | STATUS_ZEROED);
        break;
    case CMD_BOOTLOADER:
        printf("COMMAND 0xB0: rebooting into USB bootloader\n");
        sleep_ms(10);           // let the message drain (watchdog is 100 ms)
        reset_usb_boot(0, 0);   // does not return
        break;
    default:
        printf("unknown COMMAND 0x%02X\n", (unsigned)cmd);
        break;
    }
}

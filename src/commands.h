/**
 * commands.h - COMMAND register handling.
 *
 * The I2C IRQ only posts the command byte; the main loop executes it, so
 * slow actions (printing, rebooting) never run in interrupt context.
 */
#pragma once

#include <stdint.h>

// IRQ side: queue a command written to REG_COMMAND.
void commands_post(uint8_t cmd);

// Main-loop side: execute any pending command.
void commands_service(void);

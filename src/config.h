/**
 * config.h - Board pins and tunable constants for the TrackbotPico encoder
 * coprocessor. Values follow the "CuBot Encoder Interface" design doc.
 */
#pragma once

// ---- Identity ---------------------------------------------------------------
#define FW_VERSION              0x01
#define WHO_AM_I_VALUE          0xE5

// ---- I2C target (I2C0 on GP4/GP5) -------------------------------------------
// The Pi 4 supplies 1.8k pull-ups, so the Pico's internal pulls stay off.
#define I2C_TARGET_ADDR         0x42
#define I2C_SDA_PIN             4
#define I2C_SCL_PIN             5
#define I2C_BAUDRATE            400000

// ---- Quadrature encoders (PIO1) ---------------------------------------------
// PIO1, not PIO0: cyw43_arch_init() claims a PIO0 state machine (and program
// space) for the wireless chip / LED, and the quadrature program must load at
// offset 0 of its PIO. Either PIO can reach any GPIO, so the pins are unchanged.
// Each encoder's B pin must be A + 1 (the PIO program reads base and base+1).
#define ENC_PIO                 pio1
#define ENC_LEFT_PIN_A          6   // left B = GP7
#define ENC_RIGHT_PIN_A         8   // right B = GP9
#define ENC_LEFT_SM             0
#define ENC_RIGHT_SM            1

// ---- Sampling / velocity ----------------------------------------------------
#define SAMPLE_PERIOD_US        1000    // 1 kHz sampler
#define VEL_WINDOW_DEFAULT_MS   20
#define VEL_WINDOW_MIN_MS       10
#define VEL_WINDOW_MAX_MS       100

// ---- Housekeeping -----------------------------------------------------------
#define WATCHDOG_TIMEOUT_MS     100
#define STATUS_BLINK_PERIOD_MS  1000    // one S (static) or M (moving) per period
#define MOTION_THRESHOLD_CPS    20      // |velocity| above this on either side = moving
#define DEBUG_PRINT_MS          1000    // USB serial status line; 0 disables

/**
 * registers.h - I2C register map and register image.
 *
 * The I2C IRQ handler talks only to this module: it sets the register
 * pointer, then reads or writes bytes with auto-increment. All multi-byte
 * values are little-endian (native on the RP2040).
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

// ---- Register addresses -----------------------------------------------------
enum {
    REG_WHO_AM_I        = 0x00, // R   u8   fixed WHO_AM_I_VALUE
    REG_FW_VERSION      = 0x01, // R   u8
    REG_STATUS          = 0x02, // R   u8   STATUS_* bits
    REG_COMMAND         = 0x03, // W   u8   CMD_* values
    REG_CONFIG          = 0x04, // R/W u8   CONFIG_* bits
    REG_VEL_WINDOW_MS   = 0x05, // R/W u8   10..100
    REG_PING            = 0x06, // R   u8   read count; increments on every read, wraps 255 -> 0

    REG_SNAPSHOT        = 0x10, // setting the pointer here latches a snapshot
    REG_TIMESTAMP_US    = 0x10, // R   u32
    REG_LEFT_COUNT      = 0x14, // R   i32
    REG_RIGHT_COUNT     = 0x18, // R   i32
    REG_LEFT_VEL        = 0x1C, // R   i32  counts/s
    REG_RIGHT_VEL       = 0x20, // R   i32  counts/s
    REG_SEQ             = 0x24, // R   u16

    REG_MAP_SIZE        = 0x26
};

// ---- STATUS bits ------------------------------------------------------------
#define STATUS_DATA_VALID       (1u << 0)
#define STATUS_WDT_RESTART      (1u << 1)   // cleared by CMD_CLEAR_STATUS
#define STATUS_ZEROED           (1u << 2)   // cleared when STATUS is read

// ---- CONFIG bits ------------------------------------------------------------
#define CONFIG_INVERT_LEFT      (1u << 0)
#define CONFIG_INVERT_RIGHT     (1u << 1)
#define CONFIG_WRITABLE_MASK    (CONFIG_INVERT_LEFT | CONFIG_INVERT_RIGHT)

// ---- COMMAND values ---------------------------------------------------------
#define CMD_ZERO_BOTH           0x01
#define CMD_ZERO_LEFT           0x02
#define CMD_ZERO_RIGHT          0x03
#define CMD_CLEAR_STATUS        0x10
#define CMD_BOOTLOADER          0xB0

// ---- Snapshot block (22 bytes at 0x10) --------------------------------------
typedef struct __attribute__((packed)) {
    uint32_t timestamp_us;
    int32_t  left_count;
    int32_t  right_count;
    int32_t  left_vel;
    int32_t  right_vel;
    uint16_t seq;
} snapshot_t;

static_assert(sizeof(snapshot_t) == 22, "snapshot must be 22 bytes");
static_assert(REG_SNAPSHOT + sizeof(snapshot_t) == REG_MAP_SIZE, "map size");

// ---- Setup ------------------------------------------------------------------

// Fill the register image with defaults; flag a watchdog restart in STATUS.
void registers_init(bool watchdog_restart);

// ---- Called from the I2C IRQ ------------------------------------------------

// Set the register pointer (first byte of a write). 0x10 latches a snapshot.
void    registers_set_pointer(uint8_t reg);

// Return the byte at the pointer and advance it (0xFF past the map end).
uint8_t registers_read_next(void);

// Apply a written byte to the register at the pointer and advance it.
void    registers_write_next(uint8_t value);

// ---- Called from the main loop / sampler ------------------------------------

// Current CONFIG bits (CONFIG_*).
uint8_t registers_get_config(void);

// Current velocity window in ms, already clamped to 10..100.
uint8_t registers_get_vel_window_ms(void);

// Current STATUS bits (STATUS_*).
uint8_t registers_get_status(void);

// Set / clear STATUS bits atomically (safe from any context).
void    registers_set_status(uint8_t bits);
void    registers_clear_status(uint8_t bits);

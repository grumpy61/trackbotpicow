/**
 * i2c_target.c - Byte-level I2C protocol, translated into register-pointer
 * operations on registers.c.
 *
 *   Write: [reg] [data...]   first byte sets the pointer, the rest are written
 *   Read:  [reg] Sr [data...] pointer set by the write, then auto-increment
 */
#include "pico/stdlib.h"
#include "pico/i2c_slave.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "config.h"
#include "registers.h"
#include "i2c_target.h"

#define I2C_PORT i2c0

static bool expect_reg_addr = true;

// Handle one I2C event: a received byte is either the register address or
// data, a request sends the next register byte, and finish resets for the
// next transaction. Runs in IRQ context: keep it short and never block.
static void i2c_event_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: {               // controller wrote a byte
        uint8_t byte = i2c_read_byte_raw(i2c);
        if (expect_reg_addr) {
            registers_set_pointer(byte);    // latches snapshot on 0x10
            expect_reg_addr = false;
        } else {
            registers_write_next(byte);
        }
        break;
    }
    case I2C_SLAVE_REQUEST:                 // controller wants a byte
        i2c_write_byte_raw(i2c, registers_read_next());
        break;
    case I2C_SLAVE_FINISH:                  // stop or repeated start
        expect_reg_addr = true;
        break;
    default:
        break;
    }
}

// Route GP4/GP5 to I2C0 with pulls off, then start the target at 0x42.
void i2c_target_init(void) {
    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_disable_pulls(I2C_SDA_PIN);        // Pi provides 1.8k pull-ups

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_disable_pulls(I2C_SCL_PIN);

    i2c_init(I2C_PORT, I2C_BAUDRATE);
    i2c_slave_init(I2C_PORT, I2C_TARGET_ADDR, &i2c_event_handler);
}

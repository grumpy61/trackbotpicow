/**
 * i2c_target.h - I2C0 target (slave) at I2C_TARGET_ADDR on GP4/GP5.
 */
#pragma once

// Configure the pins and start answering on the bus. Call last during
// startup, once the register image and sampler are ready.
void i2c_target_init(void);

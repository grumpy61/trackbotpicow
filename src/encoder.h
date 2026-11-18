/**
 * encoder.h - PIO quadrature decoding for the two track motors.
 */
#pragma once

#include <stdint.h>

typedef enum {
    ENC_LEFT  = 0,
    ENC_RIGHT = 1,
} encoder_id_t;

// Configure the encoder GPIOs and start one PIO state machine per encoder.
void encoder_init(void);

// Raw, free-running 4x count from the PIO (no offset or inversion applied).
int32_t encoder_read_raw(encoder_id_t id);

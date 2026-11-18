/**
 * sampler.h - 1 kHz sampling of the encoders into a "live" state.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t timestamp_us;  // time_us_32() at the sample
    int32_t  left_count;    // offset-corrected, inversion applied
    int32_t  right_count;
    int32_t  left_vel;      // counts/s over VEL_WINDOW_MS
    int32_t  right_vel;
} encoder_state_t;

// Start the repeating sample timer. Call after encoder_init().
bool sampler_init(void);

// Consistent copy of the latest sample (safe from IRQ or main loop).
void sampler_get_live(encoder_state_t *out);

// Ask the sampler to zero one or both counters on its next tick.
void sampler_request_zero(bool left, bool right);

/**
 * sampler.c - 1 kHz repeating timer: raw counts -> offsets -> inversion ->
 * velocity -> `live` struct.
 *
 * Zeroing is done with software offsets so the PIO counters never restart
 * and no edge is lost.
 */
#include "pico/stdlib.h"
#include "pico/sync.h"

#include "config.h"
#include "encoder.h"
#include "registers.h"
#include "sampler.h"

#define ZERO_LEFT   (1u << 0)
#define ZERO_RIGHT  (1u << 1)

static repeating_timer_t sample_timer;
static spin_lock_t      *live_lock;
static encoder_state_t   live;

static int32_t           offset_left;
static int32_t           offset_right;
static volatile uint8_t  zero_request;

// ---- Velocity ---------------------------------------------------------------

// Forget velocity history (after a zero, so the jump isn't seen as speed).
static void velocity_reset(void) {
    // TODO: clear the ring buffer of (timestamp, left, right) samples.
}

// Push one sample and compute counts/s over the VEL_WINDOW_MS window.
static void velocity_update(uint32_t now_us, int32_t left, int32_t right,
                            uint8_t window_ms,
                            int32_t *left_vel, int32_t *right_vel) {
    // TODO: ring buffer sized for VEL_WINDOW_MAX_MS samples at 1 kHz.
    //   vel = (count_now - count_window_ago) * 1e6 / (t_now - t_window_ago)
    (void)now_us; (void)left; (void)right; (void)window_ms;
    *left_vel = 0;
    *right_vel = 0;
}

// ---- Timer callback (IRQ context) -------------------------------------------

// a - b with 32-bit wrap-around, done unsigned so overflow is well defined.
static inline int32_t wrap_sub(int32_t a, int32_t b) {
    return (int32_t)((uint32_t)a - (uint32_t)b);
}

// One 1 kHz sample: read raw counts, apply pending zeroing, offsets and
// inversion, update velocity, then publish everything to `live` under the lock.
static bool sample_tick(repeating_timer_t *t) {
    (void)t;
    uint32_t now   = time_us_32();
    int32_t  raw_l = encoder_read_raw(ENC_LEFT);
    int32_t  raw_r = encoder_read_raw(ENC_RIGHT);

    uint8_t zr = zero_request;
    if (zr) {
        if (zr & ZERO_LEFT)  offset_left  = raw_l;
        if (zr & ZERO_RIGHT) offset_right = raw_r;
        zero_request = 0;
        velocity_reset();
        registers_set_status(STATUS_ZEROED);
    }

    int32_t left  = wrap_sub(raw_l, offset_left);
    int32_t right = wrap_sub(raw_r, offset_right);

    uint8_t cfg = registers_get_config();
    if (cfg & CONFIG_INVERT_LEFT)  left  = -left;
    if (cfg & CONFIG_INVERT_RIGHT) right = -right;

    int32_t left_vel, right_vel;
    velocity_update(now, left, right, registers_get_vel_window_ms(),
                    &left_vel, &right_vel);

    uint32_t irq = spin_lock_blocking(live_lock);
    live.timestamp_us = now;
    live.left_count   = left;
    live.right_count  = right;
    live.left_vel     = left_vel;
    live.right_vel    = right_vel;
    spin_unlock(live_lock, irq);

    registers_set_status(STATUS_DATA_VALID);
    return true;    // keep repeating
}

// ---- Public API -------------------------------------------------------------

// Claim the spin lock protecting `live` and start the 1 kHz timer.
// Returns false if no timer slot was available.
bool sampler_init(void) {
    live_lock = spin_lock_instance(spin_lock_claim_unused(true));
    velocity_reset();
    // Negative period = fixed start-to-start spacing, not delay-after-callback.
    return add_repeating_timer_us(-SAMPLE_PERIOD_US, sample_tick, NULL,
                                  &sample_timer);
}

// Copy the latest sample out under the lock so all fields match.
void sampler_get_live(encoder_state_t *out) {
    uint32_t irq = spin_lock_blocking(live_lock);
    *out = live;
    spin_unlock(live_lock, irq);
}

// Flag one or both counters to be zeroed on the next tick (the tick owns the
// offsets). Interrupts are off so the flag update can't race the tick.
void sampler_request_zero(bool left, bool right) {
    uint32_t irq = save_and_disable_interrupts();
    if (left)  zero_request |= ZERO_LEFT;
    if (right) zero_request |= ZERO_RIGHT;
    restore_interrupts(irq);
}

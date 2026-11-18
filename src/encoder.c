/**
 * encoder.c - PIO quadrature decoding (ENC_PIO = PIO1, SM0 = left, SM1 = right).
 *
 * SKELETON: the GPIO setup is in place; the PIO program is not loaded yet.
 */
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "config.h"
#include "encoder.h"

// Make an encoder's A pin and the B pin after it plain inputs with no pulls.
static void encoder_pins_init(uint pin_a) {
    for (uint pin = pin_a; pin <= pin_a + 1; pin++) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        // The 10k/20k divider sets the level; the default pull-down would skew it.
        gpio_disable_pulls(pin);
    }
}

// Set up both encoders' pins and (TODO) start their PIO state machines.
void encoder_init(void) {
    encoder_pins_init(ENC_LEFT_PIN_A);
    encoder_pins_init(ENC_RIGHT_PIN_A);

    // TODO: copy quadrature_encoder.pio from pico-examples into src/, add
    //   pico_generate_pio_header(TrackbotPico ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)
    // to CMakeLists.txt, then:
    //   #include "quadrature_encoder.pio.h"
    //   pio_add_program_at_offset(ENC_PIO, &quadrature_encoder_program, 0);  // must be offset 0
    //   pio_sm_claim(ENC_PIO, ENC_LEFT_SM);
    //   pio_sm_claim(ENC_PIO, ENC_RIGHT_SM);
    //   quadrature_encoder_program_init(ENC_PIO, ENC_LEFT_SM,  ENC_LEFT_PIN_A,  0);
    //   quadrature_encoder_program_init(ENC_PIO, ENC_RIGHT_SM, ENC_RIGHT_PIN_A, 0);
}

// Return the PIO's free-running count for one encoder (0 until PIO is wired up).
int32_t encoder_read_raw(encoder_id_t id) {
    (void)id;
    // TODO: return quadrature_encoder_get_count(ENC_PIO, id == ENC_LEFT ? ENC_LEFT_SM : ENC_RIGHT_SM);
    return 0;
}

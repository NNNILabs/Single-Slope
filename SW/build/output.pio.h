// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ----- //
// count //
// ----- //

#define count_wrap_target 0
#define count_wrap 10

static const uint16_t count_program_instructions[] = {
            //     .wrap_target
    0xa02b, //  0: mov    x, !null        side 0     
    0x6040, //  1: out    y, 32           side 0     
    0x10c6, //  2: jmp    pin, 6          side 1     
    0xb142, //  3: nop                    side 1 [1] 
    0x1082, //  4: jmp    y--, 2          side 1     
    0x000a, //  5: jmp    10              side 0     
    0xb042, //  6: nop                    side 1     
    0x1048, //  7: jmp    x--, 8          side 1     
    0x1082, //  8: jmp    y--, 2          side 1     
    0x000a, //  9: jmp    10              side 0     
    0x4020, // 10: in     x, 32           side 0     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program count_program = {
    .instructions = count_program_instructions,
    .length = 11,
    .origin = -1,
};

static inline pio_sm_config count_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + count_wrap_target, offset + count_wrap);
    sm_config_set_sideset(&c, 1, false, false);
    return c;
}

void count_program_init(PIO pio, uint sm, uint offset, uint inpin, uint outpin, float div) {
    pio_sm_config c = count_program_get_default_config(offset);
    pio_gpio_init(pio, inpin);
    pio_gpio_init(pio, outpin);
    pio_sm_set_consecutive_pindirs(pio, sm, inpin, 1, false);
    pio_sm_set_consecutive_pindirs(pio, sm, outpin, 1, true);
    sm_config_set_jmp_pin(&c, inpin);
    sm_config_set_sideset_pins(&c, outpin);
    sm_config_set_clkdiv(&c, div);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_out_shift(&c, false, true, 32);
    pio_sm_init(pio, sm, offset, &c);
}

#endif


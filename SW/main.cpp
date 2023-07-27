#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

#include "output.pio.h"

#define outputPin 0
#define inputPin  1

void core2()
{
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    while (true)
    {
        gpio_put(25, 1);
        sleep_ms(500);
        gpio_put(25, 0);
        sleep_ms(500);
    }
    
}

int main() 
{
    stdio_init_all();

    //Housekeeping :amsmiles:
    vreg_set_voltage(VREG_VOLTAGE_MAX);
    set_sys_clock_khz(400000, true);

    PIO pio = pio0;
    PIO pioC = pio1;

    uint smPulser = pio_claim_unused_sm(pio, true);
    uint offsetPulser = pio_add_program(pio, &output_program);
    float divPulser = 1;
    output_program_init(pio, smPulser, offsetPulser, outputPin, divPulser);
    pio_sm_set_enabled(pio, smPulser, true);

    uint smCounter = pio_claim_unused_sm(pioC, true);
    uint offsetCounter = pio_add_program(pioC, &input_program);
    float divCounter = 1;
    input_program_init(pioC, smCounter, offsetCounter, inputPin, divCounter);
    pio_sm_set_enabled(pioC, smCounter, true);

    multicore_launch_core1(core2);
    
    uint32_t pulseWidth = 400;

    while (true) 
    {
        //pio_sm_put_blocking(pio, smPulser, (pulseWidth-1));
        printf("Count: %d\n", ~pio_sm_get_blocking(pioC, smCounter));
    }
}
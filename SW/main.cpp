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

    PIO pioPulser = pio0;
    uint smPulser = pio_claim_unused_sm(pioPulser, true);
    uint offsetPulser = pio_add_program(pioPulser, &output_program);
    float divPulser = 1;
    output_program_init(pioPulser, smPulser, offsetPulser, outputPin, divPulser);

    multicore_launch_core1(core2);
    
    uint32_t pulseWidth = 10;

    while (true) 
    {
        pio_sm_put_blocking(pioPulser, smPulser, pulseWidth);
        sleep_ms(100);
    }
}
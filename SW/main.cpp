#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

#include "output.pio.h"

#define outputPin 12
#define inputPin  0

#define inputMuxPin 13 //TMUX6234 SEL2
#define refMuxPin   14 //TMUX6234 SEL3
#define gndMuxPin   15 //TMUX6234 SEL4

void setMuxIn()
{
    gpio_put(inputMuxPin, 1);
    gpio_put(refMuxPin, 0);
    gpio_put(gndMuxPin, 0);
}

void setMuxRef()
{
    gpio_put(inputMuxPin, 0);
    gpio_put(refMuxPin, 1);
    gpio_put(gndMuxPin, 0);
}

void setMuxGnd()
{
    gpio_put(inputMuxPin, 0);
    gpio_put(refMuxPin, 0);
    gpio_put(gndMuxPin, 1);
}

void resetMux()
{
    gpio_put(inputMuxPin, 0);
    gpio_put(refMuxPin, 0);
    gpio_put(gndMuxPin, 0);
}

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

    gpio_init(inputMuxPin);
    gpio_init(refMuxPin);
    gpio_init(gndMuxPin);

    gpio_set_dir(inputMuxPin, GPIO_OUT);
    gpio_set_dir(refMuxPin, GPIO_OUT);
    gpio_set_dir(gndMuxPin, GPIO_OUT);

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
    
    uint32_t pulseWidth = 8000000;

    float reading;

    while (true) 
    {
        uint32_t zero = 0;
        uint32_t vref = 0;
        uint32_t num = 0;
        uint32_t den = 0;
        uint32_t input = 0;

        float result = 0;
        
        pio_sm_clear_fifos(pioC, smCounter);

        // Zero reading
        pio_sm_put_blocking(pio, smPulser, (pulseWidth-1));
        sleep_ms(30);
        zero = ~pio_sm_get_blocking(pioC, smCounter);
        sleep_ms(5);
        setMuxRef();
        sleep_ms(5);

        // Vref reading
        pio_sm_put_blocking(pio, smPulser, (pulseWidth-1));
        sleep_ms(30);
        vref = ~pio_sm_get_blocking(pioC, smCounter);
        sleep_ms(5);
        setMuxIn();
        sleep_ms(5);

        // Input reading
        pio_sm_put_blocking(pio, smPulser, (pulseWidth - 1));
        sleep_ms(30);
        input = ~pio_sm_get_blocking(pioC, smCounter);
        sleep_ms(5);
        setMuxGnd();
        sleep_ms(5);

        result = (float)(input - zero)/(float)(vref - zero);

        printf("%d, %d, %d, %f\n", zero, vref, input, result);
    }
}
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/gpio.h"

#include "output.pio.h"

#define outputPin   19 // 20ms conversion pulse (TMUX7234 SW2)
#define inputPin    15 // Pulse in

#define inputMuxPin 16 // TMUX7234 SW3
#define refMuxPin   18 // TMUX7234 SW1
#define gndMuxPin   17 // TMUX7234 SW4

#define NAVG        20 // Number of readings to average over

PIO pio = pio0;
uint smCount;
uint offsetCount;
float divCount;

int32_t zero = 0;
int32_t vref = 0;
int32_t input = 0;
double result = 0.0;
double vrefAbs = 7.06004;

uint32_t pulseWidth = 2000000;

void setMuxIn()
{
    gpio_put(inputMuxPin, 1);
    sleep_us(1);
    gpio_put(refMuxPin, 0);
    sleep_us(1);
    gpio_put(gndMuxPin, 0);
    sleep_us(1);
    
}

void setMuxRef()
{
    gpio_put(inputMuxPin, 0);
    sleep_us(1);
    gpio_put(refMuxPin, 1);
    sleep_us(1);
    gpio_put(gndMuxPin, 0);
    sleep_us(1);
    
}

void setMuxGnd()
{
    gpio_put(inputMuxPin, 0);
    sleep_us(1);
    gpio_put(refMuxPin, 0);
    sleep_us(1);
    gpio_put(gndMuxPin, 1);
    sleep_us(1);
    
}

void resetMux()
{
    gpio_put(inputMuxPin, 0);
    sleep_us(1);
    gpio_put(refMuxPin, 0);
    sleep_us(1);
    gpio_put(gndMuxPin, 0);
    sleep_us(1);
}

void getReading()
{
    pio_sm_clear_fifos(pio, smCount);

    gpio_put(25, 1);

    // Reference reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    sleep_ms(30);
    zero = ~pio_sm_get_blocking(pio, smCount);
    sleep_ms(5);
    setMuxRef();
    sleep_ms(5);

    // Input reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    sleep_ms(30);
    vref = ~pio_sm_get_blocking(pio, smCount);
    sleep_ms(5);
    setMuxIn();
    sleep_ms(5);

    // GND reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    sleep_ms(30);
    input = ~pio_sm_get_blocking(pio, smCount);
    sleep_ms(5);
    setMuxGnd();
    sleep_ms(5);

    gpio_put(25, 0);
}

void core2()
{

}

int main() 
{
    stdio_init_all();

    // Housekeeping :amsmiles:
    vreg_set_voltage(VREG_VOLTAGE_MAX);
    set_sys_clock_khz(400000, true);

    // Voltage regulator in PWM mode
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, 1);

    // LED reading indicator
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    gpio_init(inputMuxPin);
    gpio_init(refMuxPin);
    gpio_init(gndMuxPin);

    gpio_set_dir(inputMuxPin, GPIO_OUT);
    gpio_set_dir(refMuxPin, GPIO_OUT);
    gpio_set_dir(gndMuxPin, GPIO_OUT);

    setMuxGnd();

    // Set power supply to PWM mode
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, 1);
    
    smCount = pio_claim_unused_sm(pio, true);
    offsetCount = pio_add_program(pio, &count_program);
    divCount = 1.0f;
    count_program_init(pio, smCount, offsetCount, inputPin, outputPin, divCount);
    pio_sm_set_enabled(pio, smCount, true);

    multicore_launch_core1(core2);

    uint32_t newInput = 0;
    char inputBuffer[32] = {0};

    while (true) 
    {

        newInput = scanf("%s", &inputBuffer, 31);

        getReading();

        result = 0.0;
        result = ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
        result = result * -1;

        // for(int i = 0; i < NAVG; i = i + 1)
        // {
        //     getReading();
        //     result = result + ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
        // }
        // result = result / NAVG;

        printf("%+f\n", result);
        
        // sleep_ms(500);
    }
}
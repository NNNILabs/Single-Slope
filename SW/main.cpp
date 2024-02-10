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

#define syncPin     10 // Sync with mains, etc.

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

bool readFlag = 0;

uint32_t pulseWidth = 2000000;

void setMuxIn()
{
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(inputMuxPin, 1);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(refMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(gndMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
}

void setMuxRef()
{
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(inputMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(refMuxPin, 1);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(gndMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
}

void setMuxGnd()
{
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(inputMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(refMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(gndMuxPin, 1);
    for(uint32_t i = 0; i < 50; i++);
}

void resetMux()
{
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(inputMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(refMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
    gpio_put(gndMuxPin, 0);
    for(uint32_t i = 0; i < 50; i++);
}

void getReading(uint gpio, uint32_t events)
{
    // Don't calculate output yet!
    readFlag = 1;

    // Note: Each for iteration takes 20ns at 400MHz
    pio_sm_clear_fifos(pio, smCount);

    gpio_put(25, 1);

    // Reference reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    for(uint32_t i = 0; i < 1500000; i++);
    zero = ~pio_sm_get_blocking(pio, smCount);
    for(uint32_t i = 0; i < 250000; i++);
    setMuxRef();
    for(uint32_t i = 0; i < 250000; i++);

    // Input reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    for(uint32_t i = 0; i < 1500000; i++);
    vref = ~pio_sm_get_blocking(pio, smCount);
    for(uint32_t i = 0; i < 250000; i++);
    setMuxIn();
    for(uint32_t i = 0; i < 250000; i++);

    // GND reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    for(uint32_t i = 0; i < 1500000; i++);
    input = ~pio_sm_get_blocking(pio, smCount);
    for(uint32_t i = 0; i < 250000; i++);
    setMuxGnd();
    for(uint32_t i = 0; i < 250000; i++);

    gpio_put(25, 0);

    result = 0.0;
    result = ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
    result = result * -1;

    // Go ahead now, lol
    readFlag = 0;
}

void core2()
{
    // Reading indicator LED
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(syncPin, GPIO_IRQ_EDGE_RISE, true, &getReading);
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
        while(readFlag);

        newInput = scanf("%s", &inputBuffer, 31);

        // result = 0.0;

        // for(int i = 0; i < NAVG; i = i + 1)
        // {
        //     getReading();
        //     result = result + ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
        // }
        // result = result / NAVG;

        printf("%+f\n", result);
        // sleep_ms(100);
    }
}
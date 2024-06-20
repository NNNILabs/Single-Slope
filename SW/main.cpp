#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/gpio.h"

#include "output.pio.h"

#include "lib/i2c_slave.h"

#define outputPin   19   // 20ms conversion pulse (TMUX7234 SW2)
#define inputPin    15   // Pulse in

#define inputMuxPin 16   // TMUX7234 SW3
#define refMuxPin   18   // TMUX7234 SW1
#define gndMuxPin   17   // TMUX7234 SW4

#define NAVG        1    // Number of readings to average over

// read and mux settling times (us), should add up to ramp time x 2
#define READ_INTERVAL   1100
#define TSET_BEFORE       50
#define TSET_AFTER       850

PIO pio = pio0;
uint smCount;
uint offsetCount;
float divCount;

int32_t zero = 0;
int32_t vref = 0;
int32_t input = 0;
double result = 0.0;
double vrefAbs = 7.06004;

// 1 count = 10ns
uint32_t pulseWidth = 100000;

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
    sleep_us(READ_INTERVAL);
    zero = ~pio_sm_get_blocking(pio, smCount);
    sleep_us(TSET_BEFORE);
    setMuxRef();
    sleep_us(TSET_AFTER);

    // Input reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    sleep_us(READ_INTERVAL);
    vref = ~pio_sm_get_blocking(pio, smCount);
    sleep_us(TSET_BEFORE);
    setMuxIn();
    sleep_us(TSET_AFTER);

    // GND reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    sleep_us(READ_INTERVAL);
    input = ~pio_sm_get_blocking(pio, smCount);
    sleep_us(TSET_BEFORE);
    setMuxGnd();
    sleep_us(TSET_AFTER);

    gpio_put(25, 0);
}

void core2()
{

}

int main() 
{
    stdio_init_all();

    i2c_init();

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
    
    smCount = pio_claim_unused_sm(pio, true);
    offsetCount = pio_add_program(pio, &count_program);
    divCount = 1.0f;
    count_program_init(pio, smCount, offsetCount, inputPin, outputPin, divCount);
    pio_sm_set_enabled(pio, smCount, true);

    // multicore_launch_core1(core2);

    // Uncomment for operation over serial
    // uint32_t newInput = 0;
    // char inputBuffer[32] = {0};

    while (true) 
    {

        // Uncomment for operation over serial
        // newInput = scanf("%s", &inputBuffer, 31);

        getReading();

        // I2C operation

        // if(regs.conversionStatus == 1)
        // {
        //     result = 0.0;

        //     for(int i = 0; i < NAVG; i = i + 1)
        //     {
        //         getReading();
        //         result = result + ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
        //     }
        //     result = result / NAVG;

        //     // getReading();
        //     // result = result + ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
            
        //     result = result * -1;
        //     regs.output = result;
        //     regs.conversionStatus = 0;
        // }
        
        // delay for debugging

        sleep_ms(100);
    }
}
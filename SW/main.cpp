#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"

#include "output.pio.h"

#define outputPin   19 // 20ms conversion pulse (TMUX7234 SW2)
#define inputPin    15 // Pulse in

<<<<<<< HEAD
#define inputMuxPin 16 // TMUX7234 SW3
#define refMuxPin   18 // TMUX7234 SW1
#define gndMuxPin   17 // TMUX7234 SW4
=======
#define inputMuxPin 13 // TMUX6234 SEL2
#define refMuxPin   14 // TMUX6234 SEL3
#define gndMuxPin   15 // TMUX6234 SEL4
>>>>>>> 0c298be106369aea0613ffb5026da950a453de78

#define averageLength 50
double average[averageLength] = {0};
int averageIndex = 0;

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
    sleep_us(1);
    gpio_put(inputMuxPin, 1);
    sleep_us(1);
    gpio_put(refMuxPin, 0);
    sleep_us(1);
    gpio_put(gndMuxPin, 0);
    sleep_us(1);
}

void setMuxRef()
{
    sleep_us(1);
    gpio_put(inputMuxPin, 0);
    sleep_us(1);
    gpio_put(refMuxPin, 1);
    sleep_us(1);
    gpio_put(gndMuxPin, 0);
    sleep_us(1);
}

void setMuxGnd()
{
    sleep_us(1);
    gpio_put(inputMuxPin, 0);
    sleep_us(1);
    gpio_put(refMuxPin, 0);
    sleep_us(1);
    gpio_put(gndMuxPin, 1);
    sleep_us(1);
}

void resetMux()
{
    sleep_us(1);
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

<<<<<<< HEAD
    // Reference reading
=======
    // Zero reading
>>>>>>> 0c298be106369aea0613ffb5026da950a453de78
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
    sleep_ms(10);

    // GND reading
    pio_sm_put_blocking(pio, smCount, (pulseWidth-1));
    sleep_ms(30);
    input = ~pio_sm_get_blocking(pio, smCount);
    sleep_ms(5);
    setMuxGnd();
    sleep_ms(10);
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

    while (true) 
    {
        getReading();
        result = ((double)(input - zero)/(double)(vref - zero))*vrefAbs;
        // average[averageIndex] = testArray[averageIndex++];
        // double averageSum = 0;
        // for(int i = 0; i < averageLength; i = i + 1)
        // {
        //     averageSum = averageSum + average[i];
        // }
        // if(averageIndex >= averageLength)
        // {
        //     averageIndex = 0;
        // }
        // averageSum = averageSum/averageLength;
        // averageSum = averageSum * vrefAbs;
        // printf("%+f\n", averageSum);
        printf("%d, %d, %d, %+03d, %+f\n", zero, vref, input, (zero - input), result);
        // sleep_ms(100);
    }
}
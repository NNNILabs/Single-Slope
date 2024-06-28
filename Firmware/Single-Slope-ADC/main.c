#include <stdio.h>
#include "pico/stdlib.h"
#include "utility.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "measurement.pio.h"

#define CHARGE_PUMP_PIN 13
#define INT_RESET_OUTPUT_PIN 14
#define COMPARATOR_PIN 11

#define IN_MUX_PIN 16
#define CAL_MUX_PIN 17

#define CHARGE_PUMP_PERIOD 666  // 666 = 1.5kHz
#define INT_RESET_PERIOD 20000  // 20000 = 50Hz
 
#define PWM_PERIOD 50
#define PWM_GAP 5

#define PWM_DIVIDER 252.0f

uint slice_num;
uint chan_num;
int pwmCount = 0;
int pwmSum = 0;

PIO pio = pio0;

uint32_t ref0v = 0;
uint32_t ref7v = 0;

float ref7V_value = 7.05f;
float volt_per_count = 0.0f;

int sumCount = 0;
float sum = 0;

int muxState = 0;

void on_pwm_wrap()
{
    // This would be a good point to switch an input mux
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(INT_RESET_OUTPUT_PIN));

    muxState++;
    if(muxState == 3)
    {
        muxState = 0;   
    }

    switch(muxState)    // 0 = 0V, 1 = REF, 2 = IN
    {
        case 0:
            gpio_put(IN_MUX_PIN, 0);
            gpio_put(CAL_MUX_PIN, 1);
            break;
        case 1:
            gpio_put(IN_MUX_PIN, 0);
            gpio_put(CAL_MUX_PIN, 0);
            break;
        case 2:
            gpio_put(IN_MUX_PIN, 1);
            gpio_put(CAL_MUX_PIN, 1);
            break;
    }
}

void core1_entry()
{
    gpio_init(IN_MUX_PIN);
    gpio_set_dir(IN_MUX_PIN, GPIO_OUT);
    gpio_put(IN_MUX_PIN, 0);   // 0 = CAL, 1 = IN

    gpio_init(CAL_MUX_PIN);
    gpio_set_dir(CAL_MUX_PIN, GPIO_OUT);
    gpio_put(CAL_MUX_PIN, 1);  // 0 = REF, 1 = 0V

    gpio_set_function(CHARGE_PUMP_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(CHARGE_PUMP_PIN);
    uint chan_num = pwm_gpio_to_channel(CHARGE_PUMP_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, PWM_DIVIDER);
    pwm_config_set_wrap(&config, CHARGE_PUMP_PERIOD);     //600Hz = 1666, 200Hz = 5000, 833 = 1.2k, 666 = 1.5k
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_config_set_phase_correct(&config, false);
    pwm_init(slice_num, &config, true);
    pwm_set_chan_level(slice_num, chan_num, CHARGE_PUMP_PERIOD/2);

    gpio_set_function(INT_RESET_OUTPUT_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(INT_RESET_OUTPUT_PIN);
    chan_num = pwm_gpio_to_channel(INT_RESET_OUTPUT_PIN);
    pwm_config_set_clkdiv(&config, PWM_DIVIDER);
    pwm_config_set_wrap(&config, INT_RESET_PERIOD);     //600Hz = 1666, 200Hz = 5000, 833 = 1.2k, 666 = 1.5k
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_config_set_phase_correct(&config, false);
    pwm_init(slice_num, &config, true);
    pwm_set_chan_level(slice_num, chan_num, INT_RESET_PERIOD/2);
    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(pwm_gpio_to_slice_num(INT_RESET_OUTPUT_PIN));
    pwm_set_irq_enabled(pwm_gpio_to_slice_num(INT_RESET_OUTPUT_PIN), true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    uint sm = pio_claim_unused_sm(pio, true);
    pio_edge_measurement_program_init(pio, sm);

    while(1)
    {
        uint32_t value = pio_sm_get_blocking(pio, sm);
        value = 4294967295 - value;
        //printf("%u\n", value);
        //printf("muxState: %i %u\n", muxState, value);

        switch(muxState)
        {
            case 0:
                ref0v = value;
                break;
            case 1:
                ref7v = value;
                volt_per_count = ref7V_value / (ref7v - ref0v);
                //printf("volt_per_count: %f\n", volt_per_count);
                break;
            case 2:
                {
                    // Input
                    int32_t signedValue = value - ref0v;
                    printf("%f\n", signedValue * volt_per_count);

                    // Optional logic to average multiple samples.
                    //sum += signedValue * volt_per_count;
                    //sumCount++;
                    //if(sumCount == 5)
                    //{           
                    //  printf("%f\n", sum / 5.0f);
                    //  sumCount = 0;
                    //  sum = 0;
                    //}
                    break;
                }
        } 
    }  
}

void main()
{
    stdio_init_all();
    led_setup();

    // Voltage regulator in PWM mode
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, 1);

    //set_sys_clock_khz(196000, true);
    set_sys_clock_khz(252000, true);
    //set_sys_clock_48mhz();

    multicore_launch_core1(core1_entry);

    while (1) {
        led(1);
        sleep_ms(250);
        led(0);
        sleep_ms(250);
    }
}

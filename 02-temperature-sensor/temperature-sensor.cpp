#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"




int main()
{

    stdio_init_all();
    //initialize temperature-sensor
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }
    while (true){

        //read raw 12-bit value
        uint16_t raw= adc_read();

        //convert to voltage
        const float conversion_factor =3.3f /(1<<12);
        float voltage=raw*conversion_factor;

        //convert voltage to temperature. See Formula:Formula: T = 27 - (Voltage - 0.706) / 0.001721
        float temperature=27.0f-(voltage-0.706f)/0.001721f;

        printf("die temperatur ist %.2f C \n" ,temperature);
        sleep_ms(1000);
    } 
}

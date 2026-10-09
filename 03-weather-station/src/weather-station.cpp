#include <stdio.h>
#include "pico/stdlib.h"
#include "bmp180.h"

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    sleep_ms(500);

    printf("\n=== Weather Station (BMP180 Module) ===\n");

    if (!bmp180_init()) {
        printf("Fehler: BMP180 nicht gefunden!\n");
    } else {
        printf("BMP180 erfolgreich initialisiert.\n");
    }

    while (true) {
        BMP180_Data data = bmp180_read();

        if (data.success) {
            printf("Temperatur: %.1f °C | Luftdruck: %.2f hPa\n", 
                   data.temperature, data.pressure);
        } else {
            printf("Fehler beim Lesen des BMP180!\n");
        }

        sleep_ms(2000);
    }
}
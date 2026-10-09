#ifndef BMP180_H
#define BMP180_H

#include <stdint.h>
#include "hardware/i2c.h"

// Konfiguration für I2C
#define BMP180_I2C_PORT i2c0
#define BMP180_SDA_PIN  0
#define BMP180_SCL_PIN  1

// Struktur für die Messergebnisse
struct BMP180_Data {
    float temperature; // in °C
    float pressure;    // in hPa
    bool success;      // true, wenn Messung erfolgreich war
};

// Öffentliche Schnittstellen
bool bmp180_init();
BMP180_Data bmp180_read();

#endif // BMP180_H
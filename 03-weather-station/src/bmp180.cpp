#include "bmp180.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define BMP180_ADDR 0x77

#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT  0xF6
#define BMP180_CMD_TEMP    0x2E
#define BMP180_CMD_PRESS   0x34

struct BMP180_Calib {
    int16_t ac1, ac2, ac3;
    uint16_t ac4, ac5, ac6;
    int16_t b1, b2;
    int16_t mb, mc, md;
};

static BMP180_Calib calib;

static int16_t read_int16(uint8_t reg) {
    uint8_t data[2];
    i2c_write_blocking(BMP180_I2C_PORT, BMP180_ADDR, &reg, 1, true);
    i2c_read_blocking(BMP180_I2C_PORT, BMP180_ADDR, data, 2, false);
    return (int16_t)((data[0] << 8) | data[1]);
}

static uint16_t read_uint16(uint8_t reg) {
    return (uint16_t)read_int16(reg);
}

static void read_calibration() {
    calib.ac1 = read_int16(0xAA);
    calib.ac2 = read_int16(0xAC);
    calib.ac3 = read_int16(0xAE);
    calib.ac4 = read_uint16(0xB0);
    calib.ac5 = read_uint16(0xB2);
    calib.ac6 = read_uint16(0xB4);
    calib.b1  = read_int16(0xB6);
    calib.b2  = read_int16(0xB8);
    calib.mb  = read_int16(0xBA);
    calib.mc  = read_int16(0xBC);
    calib.md  = read_int16(0xBE);
}

bool bmp180_init() {
    i2c_init(BMP180_I2C_PORT, 100 * 1000);
    gpio_set_function(BMP180_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BMP180_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BMP180_SDA_PIN);
    gpio_pull_up(BMP180_SCL_PIN);

    // Prüfen, ob der Sensor antwortet
    uint8_t dummy;
    int ret = i2c_read_timeout_us(BMP180_I2C_PORT, BMP180_ADDR, &dummy, 1, false, 10000);
    if (ret < 0) {
        return false; // Sensor nicht gefunden
    }

    read_calibration();
    return true;
}

BMP180_Data bmp180_read() {
    BMP180_Data result = {0.0f, 0.0f, false};

    // Unkompensierte Temperatur
    uint8_t cmd_temp[2] = {BMP180_REG_CONTROL, BMP180_CMD_TEMP};
    i2c_write_blocking(BMP180_I2C_PORT, BMP180_ADDR, cmd_temp, 2, false);
    sleep_ms(5);

    uint8_t reg = BMP180_REG_RESULT;
    uint8_t data_temp[2];
    i2c_write_blocking(BMP180_I2C_PORT, BMP180_ADDR, &reg, 1, true);
    i2c_read_blocking(BMP180_I2C_PORT, BMP180_ADDR, data_temp, 2, false);
    int32_t ut = (int32_t)((data_temp[0] << 8) | data_temp[1]);

    // Unkompensierter Druck
    uint8_t cmd_press[2] = {BMP180_REG_CONTROL, BMP180_CMD_PRESS};
    i2c_write_blocking(BMP180_I2C_PORT, BMP180_ADDR, cmd_press, 2, false);
    sleep_ms(5);

    uint8_t data_press[2];
    i2c_write_blocking(BMP180_I2C_PORT, BMP180_ADDR, &reg, 1, true);
    i2c_read_blocking(BMP180_I2C_PORT, BMP180_ADDR, data_press, 2, false);
    int32_t up = (int32_t)((data_press[0] << 8) | data_press[1]);

    // Berechnung Temperatur
    int32_t x1 = ((ut - (int32_t)calib.ac6) * (int32_t)calib.ac5) >> 15;
    int32_t x2 = ((int32_t)calib.mc << 11) / (x1 + calib.md);
    int32_t b5 = x1 + x2;
    result.temperature = ((b5 + 8) >> 4) / 10.0f;

    // Berechnung Druck
    int32_t b6 = b5 - 4000;
    x1 = (calib.b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = (calib.ac2 * b6) >> 11;
    int32_t x3 = x1 + x2;
    int32_t b3 = (((((int32_t)calib.ac1) * 4 + x3)) + 2) >> 2;

    x1 = (calib.ac3 * b6) >> 13;
    x2 = (calib.b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    uint32_t b4 = (calib.ac4 * (uint32_t)(x3 + 32768)) >> 15;
    uint32_t b7 = ((uint32_t)up - b3) * 50000;

    int32_t p;
    if (b7 < 0x80000000) {
        p = (b7 * 2) / b4;
    } else {
        p = (b7 / b4) * 2;
    }

    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p = p + ((x1 + x2 + 3791) >> 4);

    result.pressure = p / 100.0f;
    result.success = true;

    return result;
}
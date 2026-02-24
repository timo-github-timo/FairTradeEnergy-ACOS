#include "io_expander.h"
#include "i2c.h"                                                    // deine I2C-Funktionen

void PCA9555_Init(void) {

    uint8_t cfg0 = (1<<0)|(1<<1)|(1<<2)|(1<<3);
    uint8_t cfg1 = (1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7);

    i2c_write(PCA9555_ADDR, REG_CONFIG_P0, &cfg0, 1);
    i2c_write(PCA9555_ADDR, REG_CONFIG_P1, &cfg1, 1);
}

void PCA9555_WritePin(PCA9555_Pin pin, bool level) {

    uint8_t port  = pin >> 8;
    uint8_t bit   = pin & 0xFF;
    uint8_t reg   = (port == 0 ? REG_OUTPUT_P0 : REG_OUTPUT_P1);
    uint8_t out;

    i2c_read(PCA9555_ADDR, reg, &out, 1);

    if (level) out |=  (1 << bit);
    else       out &= ~(1 << bit);

    i2c_write(PCA9555_ADDR, reg, &out, 1);
}

bool PCA9555_ReadPin(PCA9555_Pin pin) {

    uint8_t port = pin >> 8;
    uint8_t bit  = pin & 0xFF;
    uint8_t reg  = (port == 0 ? REG_INPUT_P0 : REG_INPUT_P1);
    uint8_t in;

    i2c_read(PCA9555_ADDR, reg, &in, 1);
    return (in & (1 << bit)) != 0;
}
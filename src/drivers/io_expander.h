#pragma once

#include <Arduino.h>


namespace pca9555 {

// I2C-Adresse
constexpr uint8_t ADDRESS = 0x20;

// Register
constexpr uint8_t REG_INPUT_P0  = 0x00;
constexpr uint8_t REG_INPUT_P1  = 0x01;
constexpr uint8_t REG_OUTPUT_P0 = 0x02;
constexpr uint8_t REG_OUTPUT_P1 = 0x03;
constexpr uint8_t REG_CONFIG_P0 = 0x06;
constexpr uint8_t REG_CONFIG_P1 = 0x07;

// Pin-Definition
enum class Pin : uint16_t {
    GRID_ON  = (0 << 8) | 4,
    ISLE_ON  = (0 << 8) | 5,
    GI_SEL   = (0 << 8) | 6,
    NTC_HOT  = (0 << 8) | 7,
    ASEL1    = (1 << 8) | 0,
    ASEL2    = (1 << 8) | 1,
};

// API
void init();
void writePin(Pin pin, bool level);
bool readPin(Pin pin);

}
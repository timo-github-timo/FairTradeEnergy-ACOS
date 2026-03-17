#include "io_expander.h"

// PCA9555 register definitions
static constexpr uint8_t PCA9555_REG_INPUT0 = 0x00;
static constexpr uint8_t PCA9555_REG_INPUT1 = 0x01;
static constexpr uint8_t PCA9555_REG_OUTPUT0 = 0x02;
static constexpr uint8_t PCA9555_REG_OUTPUT1 = 0x03;
static constexpr uint8_t PCA9555_REG_POLARITY0 = 0x04;
static constexpr uint8_t PCA9555_REG_POLARITY1 = 0x05;
static constexpr uint8_t PCA9555_REG_CONFIG0 = 0x06;
static constexpr uint8_t PCA9555_REG_CONFIG1 = 0x07;

static uint8_t toReg(uint8_t pin) {
    return (pin < 8) ? pin : (pin - 8);
}

bool IOExpander::begin(uint8_t i2cAddr, TwoWire &wire) {
    this->wire = &wire;
    this->i2cAddr = i2cAddr;

    wire.begin();

    // Set all pins to inputs by default (1 = input in config register)
    writeRegister(PCA9555_REG_CONFIG0, 0xFF);
    writeRegister(PCA9555_REG_CONFIG1, 0xFF);

    // Clear outputs as a safe default
    outputState = 0;
    writeRegister(PCA9555_REG_OUTPUT0, 0x00);
    writeRegister(PCA9555_REG_OUTPUT1, 0x00);

    // Read back config to confirm device is present
    uint8_t cfg0 = readRegister(PCA9555_REG_CONFIG0);
    uint8_t cfg1 = readRegister(PCA9555_REG_CONFIG1);

    return (cfg0 == 0xFF && cfg1 == 0xFF);
}

void IOExpander::pinMode(uint8_t pin, uint8_t mode) {
    if (pin > 15) {
        return;
    }

    uint8_t reg = (pin < 8) ? PCA9555_REG_CONFIG0 : PCA9555_REG_CONFIG1;
    uint8_t bit = 1 << toReg(pin);

    uint8_t cfg = readRegister(reg);

    if (mode == OUTPUT) {
        cfg &= ~bit;
        direction &= ~((uint16_t)1 << pin);
    } else {
        cfg |= bit;
        direction |= ((uint16_t)1 << pin);
    }

    writeRegister(reg, cfg);
}

void IOExpander::digitalWrite(uint8_t pin, uint8_t value) {
    if (pin > 15) {
        return;
    }

    // If pin is configured as input, behave like Arduino (no-op)
    if (direction & ((uint16_t)1 << pin)) {
        return;
    }

    uint8_t reg = (pin < 8) ? PCA9555_REG_OUTPUT0 : PCA9555_REG_OUTPUT1;

    if (value == LOW) {
        outputState &= ~((uint16_t)1 << pin);
    } else {
        outputState |= ((uint16_t)1 << pin);
    }

    uint8_t out = (uint8_t)(outputState >> ((pin < 8) ? 0 : 8));
    writeRegister(reg, out);
}

int IOExpander::digitalRead(uint8_t pin) {
    if (pin > 15) {
        return LOW;
    }

    uint8_t reg = (pin < 8) ? PCA9555_REG_INPUT0 : PCA9555_REG_INPUT1;
    uint8_t bit = 1 << toReg(pin);

    uint8_t val = readRegister(reg);
    return (val & bit) ? HIGH : LOW;
}

uint16_t IOExpander::readPort() {
    uint16_t low = readRegister(PCA9555_REG_INPUT0);
    uint16_t high = readRegister(PCA9555_REG_INPUT1);
    return (high << 8) | low;
}

void IOExpander::writePort(uint16_t value) {
    outputState = value;
    writeRegister(PCA9555_REG_OUTPUT0, (uint8_t)(value & 0xFF));
    writeRegister(PCA9555_REG_OUTPUT1, (uint8_t)(value >> 8));
}

uint8_t IOExpander::readRegister(uint8_t reg) {
    wire->beginTransmission(i2cAddr);
    wire->write(reg);
    wire->endTransmission();

    wire->requestFrom(i2cAddr, (uint8_t)1);
    if (wire->available()) {
        return wire->read();
    }
    return 0;
}

void IOExpander::writeRegister(uint8_t reg, uint8_t value) {
    wire->beginTransmission(i2cAddr);
    wire->write(reg);
    wire->write(value);
    wire->endTransmission();
}

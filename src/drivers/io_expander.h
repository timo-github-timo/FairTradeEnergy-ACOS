#pragma once

#include <Arduino.h>
#include <Wire.h>

/**
 * Basic PCA9555 I2C I/O Expander driver.
 *
 * This driver provides a minimal subset of GPIO functionality needed for the
 * project. It supports setting pin direction (INPUT/OUTPUT) and reading/writing
 * values on the 16 GPIOs.
 */
class IOExpander {
public:
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x20;

    /**
     * Initialize the expander.
     *
     * @param i2cAddr I2C address of the PCA9555 (default 0x20).
     * @param wire The TwoWire instance to use (defaults to Wire).
     * @return true if the device appears to be present.
     */
    bool begin(uint8_t i2cAddr = DEFAULT_I2C_ADDRESS, TwoWire &wire = Wire);

    /**
     * Configure a pin as INPUT or OUTPUT.
     */
    void pinMode(uint8_t pin, uint8_t mode);

    /**
     * Write a digital level to a pin configured as output.
     */
    void digitalWrite(uint8_t pin, uint8_t value);

    /**
     * Read a digital level from a pin configured as input (or output).
     */
    int digitalRead(uint8_t pin);

    /**
     * Read the full 16-bit port state.
     */
    uint16_t readPort();

    /**
     * Write the full 16-bit port state (only affects pins configured as outputs).
     */
    void writePort(uint16_t value);

private:
    TwoWire *wire = nullptr;
    uint8_t i2cAddr = DEFAULT_I2C_ADDRESS;

    uint16_t outputState = 0; // last written output state
    uint16_t direction = 0xFFFF; // 1 = input, 0 = output (default all inputs)

    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
};

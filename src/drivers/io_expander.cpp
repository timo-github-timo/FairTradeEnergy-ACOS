#include "io_expander.h"
#include "i2c.h"  // deine i2c_read/i2c_write

namespace pca9555 {

void init() {
    // Port0: Bits 0–3 Input (1), Bits 4–7 Output (0)
    uint8_t cfg0 = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);

    // Port1: Beispiel: Bits 0–1 Output, Rest Input
    // (du hattest ab Bit2 Input gesetzt, das bleibt so)
    uint8_t cfg1 = (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7);

    i2c_write(ADDRESS, REG_CONFIG_P0, &cfg0, 1);
    i2c_write(ADDRESS, REG_CONFIG_P1, &cfg1, 1);
}

void writePin(Pin pin, bool level) {
    const uint16_t raw = static_cast<uint16_t>(pin);
    const uint8_t port = static_cast<uint8_t>(raw >> 8);
    const uint8_t bit  = static_cast<uint8_t>(raw & 0xFF);

    const uint8_t reg  = (port == 0) ? REG_OUTPUT_P0 : REG_OUTPUT_P1;
    uint8_t out = 0;

    // aktuellen Ausgangswert holen
    i2c_read(ADDRESS, reg, &out, 1);

    // Bit setzen oder löschen
    if (level) out |=  (1u << bit);
    else       out &= ~(1u << bit);

    // zurückschreiben
    i2c_write(ADDRESS, reg, &out, 1);
}

bool readPin(Pin pin) {
    const uint16_t raw = static_cast<uint16_t>(pin);
    const uint8_t port = static_cast<uint8_t>(raw >> 8);
    const uint8_t bit  = static_cast<uint8_t>(raw & 0xFF);

    const uint8_t reg  = (port == 0) ? REG_INPUT_P0 : REG_INPUT_P1;
    uint8_t in = 0;

    i2c_read(ADDRESS, reg, &in, 1);
    return (in & (1u << bit)) != 0;
}

} // namespace pca9555
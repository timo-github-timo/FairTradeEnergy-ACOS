// I2C-Adresse des PCA9555
#define PCA9555_ADDR    0x20

// Register-Adressen
#define REG_INPUT_P0    0x00
#define REG_INPUT_P1    0x01
#define REG_OUTPUT_P0   0x02
#define REG_OUTPUT_P1   0x03
#define REG_CONFIG_P0   0x06
#define REG_CONFIG_P1   0x07

// Funktionen und ihr Port/Bit
typedef enum {
  PIN_GRID_ON  = (0<<8) | 4,  // Port 0, Bit 4
  PIN_ISLE_ON  = (0<<8) | 5,  
  PIN_GI_SEL   = (0<<8) | 6,  
  PIN_NTC_HOT  = (0<<8) | 7,  
  PIN_ASEL1    = (1<<8) | 0,  // Port 1, Bit 0
  PIN_ASEL2    = (1<<8) | 1,  
  // … weitere Belegungen
} PCA9555_Pin;


// Initialisierung des PCA9555: Konfiguration der Ports als Ein- oder Ausgang
void PCA9555_Init(void) {
    // Port0: Bits 4–7 als Ausgang, Bits 0–3 als Eingang
    uint8_t cfg0 = (1<<0)|(1<<1)|(1<<2)|(1<<3)  // 1 = Input
                 |(0<<4)|(0<<5)|(0<<6)|(0<<7); // 0 = Output
    // Port1: z. B. Bits 0–1 als Ausgang, Rest als Eingang
    uint8_t cfg1 = (1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7);

    i2c_write(PCA9555_ADDR, REG_CONFIG_P0, &cfg0, 1);
    i2c_write(PCA9555_ADDR, REG_CONFIG_P1, &cfg1, 1);
}


// Lese und Schreibroutinen für die GPIOs des PCA9555

// Ein einzelnes GPIO schreiben
void PCA9555_WritePin(PCA9555_Pin pin, bool level) {
    uint8_t port  = pin >> 8;
    uint8_t bit   = pin & 0xFF;
    uint8_t reg   = (port == 0 ? REG_OUTPUT_P0 : REG_OUTPUT_P1);
    uint8_t out;

    // aktuellen Ausgangswert holen
    i2c_read(PCA9555_ADDR, reg, &out, 1);

    // Bit setzen oder löschen
    if (level) out |=  (1 << bit);
    else       out &= ~(1 << bit);

    // zurückschreiben
    i2c_write(PCA9555_ADDR, reg, &out, 1);
}

// Ein einzelnes GPIO lesen
bool PCA9555_ReadPin(PCA9555_Pin pin) {
    uint8_t port = pin >> 8;
    uint8_t bit  = pin & 0xFF;
    uint8_t reg  = (port == 0 ? REG_INPUT_P0 : REG_INPUT_P1);
    uint8_t in;

    i2c_read(PCA9555_ADDR, reg, &in, 1);
    return (in & (1 << bit)) != 0;
}

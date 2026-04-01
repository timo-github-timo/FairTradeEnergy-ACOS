#pragma once
#include <Arduino.h>

// ================================================================
//  config.h  —  Globale Konstanten, Pin-Definitionen, Enums, Structs
//  Zielplattform: M5CoreS3SE (ESP32-S3), Stecker ST1 (IDC30)
//
//  HINWEIS: G-Nummern aus der Schaltplan-Netzliste.
//           Zuordnung ST1-Pin <-> ESP32-GPIO anhand M5CoreS3SE-Pinout prüfen!
//           Pins ohne explizite G-Nummer (CAN_CS, CAN_INT, RS_WR) sind
//           als Platzhalter eingetragen — vor Inbetriebnahme verifizieren.
// ================================================================

// ----------------------------------------------------------------
//  I²C (PCA9555 IC9, Pull-ups R39/R40 auf Platine)
// ----------------------------------------------------------------
#define PIN_I2C_SDA         12      // G12  — ST1 Pin 14 : SDA
#define PIN_I2C_SCL         11      // G11  — ST1 Pin 13 : SCL
#define PIN_PCA9555_INT      1      // G1   — ST1 Pin 11 : IC9 INT (active-LOW, open-drain)

// ----------------------------------------------------------------
//  ADC-Eingang (IC14 Analog-Multiplexer Ausgang)
// ----------------------------------------------------------------
#define PIN_ADC             10      // G10  — ST1 Pin 29 : ADC-Eingang

// ----------------------------------------------------------------
//  Optokoppler-Eingänge  (active-LOW, INPUT_PULLUP)
// ----------------------------------------------------------------
#define PIN_nVGRID           7      // G7   — ST1 Pin 7  : !VGRID    Netzspannung vorhanden
#define PIN_nVGRID_OV        5      // G5   — ST1 Pin 5  : !VGRID_OV Netz-Überspannung
#define PIN_nVLOAD           8      // G8   — ST1 Pin 8  : !VLOAD    Lastspannung vorhanden
#define PIN_nVISLE          21      // G21  — ST1 Pin 21 : !VISLE    Inselspannung vorhanden

// ----------------------------------------------------------------
//  CAN-Bus SPI (MCP2515)
// ----------------------------------------------------------------
#define PIN_CAN_MOSI        37      // G37  — ST1 Pin 24
#define PIN_CAN_MISO        35      // G35  — ST1 Pin 22
#define PIN_CAN_SCK         36      // G36  — ST1 Pin 20
#define PIN_CAN_CS          38      // TODO — ST1 Pin 23, GPIO aus M5CoreS3SE-Pinout verifizieren
#define PIN_CAN_INT         45      // TODO — ST1 Pin 12, GPIO aus M5CoreS3SE-Pinout verifizieren

// ----------------------------------------------------------------
//  RS485 (IC5 ISOW1412, Hardware vorhanden — in v1.0 nicht implementiert)
// ----------------------------------------------------------------
#define PIN_RS485_TX        18      // G18  — ST1 Pin 15
#define PIN_RS485_RX        17      // G17  — ST1 Pin 16
#define PIN_RS485_WR        46      // TODO — ST1 Pin 10, GPIO aus M5CoreS3SE-Pinout verifizieren

// ----------------------------------------------------------------
//  ADC-Multiplexer Kanal-IDs  (ASEL1/ASEL2 via PCA9555 IO0_4/IO0_5)
// ----------------------------------------------------------------
#define ADC_CH_GRID          0      // ASEL2=0, ASEL1=0 — Netzspannung  (TR2 -> IC11/IC12 -> IC14)
#define ADC_CH_BATTERY       1      // ASEL2=0, ASEL1=1 — Batteriespannung (R59/R60 Teiler)
#define ADC_CH_LOAD          2      // ASEL2=1, ASEL1=0 — Lastspannung

// ----------------------------------------------------------------
//  ADC-Skalierung
// ----------------------------------------------------------------
#define ADC_RESOLUTION      4095.0f // 12-Bit
#define ADC_VREF            3.3f    // V — Referenz
#define ADC_BATT_DIVIDER    101.0f  // R59(10MΩ) / R60(100kΩ): (10M+100k)/100k = 101
#define ADC_GRID_SCALE      150.0f  // Platzhalter — an realer Hardware kalibrieren!
#define ADC_OVERSAMPLING    16      // Samples pro Messung (Rauschunterdrückung)
#define ADC_SETTLING_US     100     // µs — Settling-Time nach Mux-Umschaltung

// ----------------------------------------------------------------
//  Schwellwerte Zustandsmaschine
// ----------------------------------------------------------------
#define THR_SOC_EMERGENCY    5      // %  — Unter diesem SOC: Notabschaltung
#define THR_SOC_MIN         15      // %  — Unter diesem SOC: Netzbetrieb bevorzugen
#define THR_SOC_MAX         95      // %  — Über diesem SOC:  Inselbetrieb bevorzugen
#define THR_SWITCH_V        50.0f   // V  — Lastspannung unter diesem Wert = spannungsfrei
#define TIMEOUT_RELAY_MS    3000    // ms — Max. Wartezeit in Transition-Zuständen

// ----------------------------------------------------------------
//  Betriebsmodi  (Touch + CAN setzen, Zustandsmaschine liest)
// ----------------------------------------------------------------
enum OperatingMode : uint8_t {
    MODE_AUTO        = 0,
    MODE_HAND_GRID   = 1,
    MODE_HAND_ISLAND = 2,
    MODE_HAND_OFF    = 3,
};

// ----------------------------------------------------------------
//  ACOS-Zustände  (Zustandsmaschine)
// ----------------------------------------------------------------
enum AcosState : uint8_t {
    STATE_OFF                  = 0,
    STATE_ISLAND               = 1,
    STATE_GRID                 = 2,
    STATE_TRANSITION_TO_GRID   = 3,
    STATE_TRANSITION_TO_ISLAND = 4,
    STATE_ERROR                = 5,
};

// ----------------------------------------------------------------
//  Sensor-Messwerte  (zwischen Modulen ausgetauscht)
// ----------------------------------------------------------------
struct SensorReadings {
    float  v_grid;   // Netzspannung     [V]
    float  v_batt;   // Batteriespannung [V]
    float  v_load;   // Lastspannung     [V]
    int8_t soc;      // SOC              [%]   EMA-gefiltert
    bool   grid_ok;  // true = Netzspannung vorhanden   (!VGRID  LOW)
    bool   grid_ov;  // true = Netz-Überspannung        (!VGRID_OV LOW)
    bool   load_on;  // true = Last hat Spannung        (!VLOAD  LOW)
    bool   isle_ok;  // true = Inselspannung vorhanden  (!VISLE  LOW)
    bool   ntc_hot;  // true = Übertemperatur (PCA9555 IO0_3, active-HIGH)
};

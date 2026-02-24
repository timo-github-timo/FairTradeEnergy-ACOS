// Pin Configuration

include <Arduino.h>
#pragma once

// ——————————————————————————————————————————————————————————
//  Pin-Definitionen (ersetze die /* TODO */-Werte durch die echten GPIO-Nummern)
// ——————————————————————————————————————————————————————————

constexpr int PIN_GI_SEL      = /* TODO: GPIO für GI_SEL */;      // Grid/Island Select
constexpr int PIN_GRID_ON     = /* TODO: GPIO für GRID_ON */;     // Netz erkannt
constexpr int PIN_ISLE_ON     = /* TODO: GPIO für ISLE_ON */;     // Inselbetrieb aktiv
constexpr int PIN_ASEL1       = /* TODO: GPIO für ASEL1 */;       // A/D Auswahl 1
constexpr int PIN_ASEL2       = /* TODO: GPIO für ASEL2 */;       // A/D Auswahl 2
constexpr int PIN_ADC         = /* TODO: GPIO für ADC */;         // A/D Eingang
constexpr int PIN_BPS_EN      = /* TODO: GPIO für BPS_EN */;      // Batterie-Unterspannung enable
constexpr int PIN_CAN_CLK     = /* TODO: GPIO für CAN_CLK */;     // CAN-Takt
constexpr int PIN_CAN_INT     = /* TODO: GPIO für CAN_INT */;     // CAN-Interrupt
constexpr int PIN_CAN_MISO    = /* TODO: GPIO für CAN_MISO */;    // CAN-MISO
constexpr int PIN_CAN_MOSI    = /* TODO: GPIO für CAN_MOSI */;    // CAN-MOSI
constexpr int PIN_CANH        = /* TODO: GPIO für CANH */;        // CAN-High
constexpr int PIN_CANL        = /* TODO: GPIO für CANL */;        // CAN-Low

constexpr int PIN_RS485A      = /* TODO: GPIO für RS485A */;      // RS485 A
constexpr int PIN_RS485B      = /* TODO: GPIO für RS485B */;      // RS485 B
constexpr int PIN_RS_RX       = /* TODO: GPIO für RS_RX */;       // RS-485 RX
constexpr int PIN_RS_TX       = /* TODO: GPIO für RS_TX */;       // RS-485 TX
constexpr int PIN_RS_WR       = /* TODO: GPIO für RS_WR */;       // RS-485 Write Enable

constexpr int PIN_SDA         = /* TODO: GPIO für SDA */;         // I²C SDA
constexpr int PIN_SCL         = /* TODO: GPIO für SCL */;         // I²C SCL
constexpr int PIN_SINT        = /* TODO: GPIO für SINT */;        // I²C-Interrupt

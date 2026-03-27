# ACOS-ChangeOverSwitch — Vereinfachte Arduino-Projektstruktur

## Empfohlene Libraries (getestet & weit verbreitet)

### 1. PCA9555 I/O-Expander
**Library:** `nicoverduin/PCA9555`  
**GitHub:** https://github.com/nicoverduin/PCA9555  
**Arduino Library Manager:** Suche nach `PCA9555` (by Nico Verduin)  
**Warum:** Nutzt exakt die gleichen Funktionen wie Arduino: `pinMode()`, `digitalRead()`, `digitalWrite()` — plus Interrupt-Support via `pinStates()`.

### 2. MCP2515 CAN Bus
**Library:** `autowp/arduino-mcp2515`  
**GitHub:** https://github.com/autowp/arduino-mcp2515  
**Arduino Library Manager:** Suche nach `MCP2515` (by autowp)  
**Warum:** De-facto-Standard für MCP2515 auf ESP32/ESP32-S3. Linux-kompatible `can_frame`-Struktur, Interrupt-basierter Empfang, Polling — beides möglich.

### 3. M5CoreS3SE Display
**Library:** `M5Unified` (Espressif/M5Stack)  
**Arduino Library Manager:** Suche nach `M5Unified`  
**Warum:** Einheitliche API für alle M5Stack-Geräte, Touch und Display in einer Library.

---

## Neue vereinfachte Projektstruktur

```
ACOS-ChangeOverSwitch-Arduino/
├── ACOS.ino              # setup() + loop() + Logik
├── config.h              # Alle Pins, PCA9555-Pins, CAN-IDs, Schwellwerte
└── README.md
```

Alles in **einer** `.ino`-Datei + `config.h` — kein `.cpp`/`.h` Splitting nötig.

---

## config.h — Vollständige Pin-Konfiguration

```cpp
#pragma once
#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════
//  HARDWARE PINS — M5CoreS3SE (ESP32-S3)
//  Format: #define PIN_NAME  GPIO-Nummer
//  Kommentar: [IN/OUT] [INT?] [ADC?] [ACTIVE_LOW/ACTIVE_HIGH]
// ═══════════════════════════════════════════════════════════════

// --- I2C Bus (Wire) ---
#define PIN_I2C_SDA        2   // [IN/OUT] I2C Data
#define PIN_I2C_SCL        1   // [IN/OUT] I2C Clock

// --- SPI Bus (für MCP2515) ---
#define PIN_SPI_MOSI      37   // [OUT] SPI MOSI
#define PIN_SPI_MISO      35   // [IN]  SPI MISO
#define PIN_SPI_SCK       36   // [OUT] SPI Clock

// --- MCP2515 CAN Controller ---
#define PIN_CAN_CS        45   // [OUT] SPI Chip Select   ACTIVE_LOW
#define PIN_CAN_INT       16   // [IN]  CAN Interrupt     ACTIVE_LOW, INT

// --- PCA9555 I/O-Expander ---
#define PCA9555_I2C_ADDR  0x20 // I2C-Adresse (A0=A1=A2=GND → 0x20)
#define PIN_PCA_INT        4   // [IN]  PCA9555 Interrupt  ACTIVE_LOW, INT

// --- ADC Eingänge (ESP32-S3 ADC1) ---
#define PIN_BATT_VOLTAGE   6   // [IN]  ADC1_CH5, Batterie-Spannung
#define PIN_BATT_CURRENT   7   // [IN]  ADC1_CH6, Batterie-Strom

// ═══════════════════════════════════════════════════════════════
//  PCA9555 PINS (0–7 = Port 0,  8–15 = Port 1)
//  Format: #define EXPPIN_NAME  Pin-Nummer (0-15)
//  Kommentar: [IN/OUT] [ACTIVE_LOW/ACTIVE_HIGH]
// ═══════════════════════════════════════════════════════════════

// --- Digitale Eingänge (Port 0, Pins 0-7) ---
#define EXPPIN_SWITCH_AC1    0  // [IN] Schalter Netz 1      ACTIVE_LOW
#define EXPPIN_SWITCH_AC2    1  // [IN] Schalter Netz 2      ACTIVE_LOW
#define EXPPIN_FAULT_AC1     2  // [IN] Fehler Netz 1        ACTIVE_LOW
#define EXPPIN_FAULT_AC2     3  // [IN] Fehler Netz 2        ACTIVE_LOW
#define EXPPIN_OVERTEMP      4  // [IN] Übertemperatur       ACTIVE_LOW
#define EXPPIN_EMERGENCY     5  // [IN] Not-Aus              ACTIVE_LOW
#define EXPPIN_IN_6          6  // [IN] Reserviert
#define EXPPIN_IN_7          7  // [IN] Reserviert

// --- Digitale Ausgänge (Port 1, Pins 8-15) ---
#define EXPPIN_RELAY_AC1     8  // [OUT] Schütz Netz 1       ACTIVE_HIGH
#define EXPPIN_RELAY_AC2     9  // [OUT] Schütz Netz 2       ACTIVE_HIGH
#define EXPPIN_LED_OK       10  // [OUT] Status-LED Grün     ACTIVE_HIGH
#define EXPPIN_LED_WARN     11  // [OUT] Status-LED Gelb     ACTIVE_HIGH
#define EXPPIN_LED_FAULT    12  // [OUT] Status-LED Rot      ACTIVE_HIGH
#define EXPPIN_BUZZER       13  // [OUT] Summer              ACTIVE_HIGH
#define EXPPIN_OUT_14       14  // [OUT] Reserviert
#define EXPPIN_OUT_15       15  // [OUT] Reserviert

// ═══════════════════════════════════════════════════════════════
//  ACTIVE-LOW / ACTIVE-HIGH HELPER MAKROS
//  Abstrahiert die Polarität — Logik immer in "aktiv = wahr"
// ═══════════════════════════════════════════════════════════════

// Physikalischen Pegel lesen → logischen Wert (true = aktiv)
#define READ_ACTIVE_LOW(pin_val)   (!(pin_val))   // LOW = aktiv
#define READ_ACTIVE_HIGH(pin_val)  (!!(pin_val))  // HIGH = aktiv

// Logischen Wert → physikalischen Pegel setzen
#define SET_ACTIVE_LOW(active)    ((active) ? LOW : HIGH)
#define SET_ACTIVE_HIGH(active)   ((active) ? HIGH : LOW)

// Kurzform für PCA9555-Ausgänge (je nach Polarität)
#define EXP_WRITE_RELAY(expander, pin, on)  (expander).digitalWrite(pin, SET_ACTIVE_HIGH(on))
#define EXP_WRITE_LED(expander, pin, on)    (expander).digitalWrite(pin, SET_ACTIVE_HIGH(on))

// ═══════════════════════════════════════════════════════════════
//  ADC KONFIGURATION
// ═══════════════════════════════════════════════════════════════
#define ADC_RESOLUTION        12        // Bits (0–4095)
#define ADC_VREF_MV         3300        // mV Referenz
#define BATT_VOLTAGE_DIVIDER   2.0f    // Spannungsteiler-Faktor
#define BATT_FULL_MV        14200       // mV = 100% SOC (14.2V)
#define BATT_EMPTY_MV       11000       // mV = 0% SOC  (11.0V)
#define BATT_WARN_SOC          20       // % Warnschwelle
#define BATT_LOW_SOC           10       // % Tiefentlade-Schutz

// ═══════════════════════════════════════════════════════════════
//  CAN BUS KONFIGURATION (MCP2515)
// ═══════════════════════════════════════════════════════════════
#define CAN_BITRATE          CAN_500KBPS   // Bitrate
#define CAN_CLOCK_FREQ       MCP_8MHZ      // Quarzfrequenz des MCP2515-Moduls

// CAN Message IDs (Standard 11-bit)
#define CAN_ID_STATUS        0x100   // ACOS → Bus: Zustandsmeldung
#define CAN_ID_SETPOINT      0x200   // Bus → ACOS: Sollwert/Steuerung
#define CAN_ID_BATTERY       0x300   // ACOS → Bus: Batterie-Werte
#define CAN_ID_FAULT         0x400   // ACOS → Bus: Fehlermeldungen

// ═══════════════════════════════════════════════════════════════
//  SYSTEMKONSTANTEN
// ═══════════════════════════════════════════════════════════════
#define LOOP_INTERVAL_MS       100   // Loop-Periode [ms]
#define DEBOUNCE_MS             20   // Entprell-Zeit [ms]
#define RELAY_SWITCH_DELAY_MS  200   // Pause zwischen Schütz aus/ein [ms]
```

---

## ACOS.ino — Vollständiges Sketch-Template

```cpp
// ═══════════════════════════════════════════════════════════════
//  ACOS - Automatic Change-Over Switch
//  Hardware: M5CoreS3SE (ESP32-S3)
//  Libraries:
//    - nicoverduin/PCA9555   (Arduino Library Manager)
//    - autowp/arduino-mcp2515 (Arduino Library Manager)
//    - M5Unified              (Arduino Library Manager)
// ═══════════════════════════════════════════════════════════════

#include "config.h"
#include <Wire.h>
#include <SPI.h>
#include <PCA9555.h>          // nicoverduin/PCA9555
#include <mcp2515.h>          // autowp/arduino-mcp2515
#include <M5Unified.h>        // M5Stack Display + Touch

// ─────────────────────────────────────────────
//  GLOBALE OBJEKTE
// ─────────────────────────────────────────────
PCA9555 expander(PCA9555_I2C_ADDR);   // I2C-Expander
MCP2515 mcp2515(PIN_CAN_CS);          // CAN Controller (CS-Pin)

// ─────────────────────────────────────────────
//  ZUSTAND
// ─────────────────────────────────────────────
enum State { STATE_IDLE, STATE_AC1, STATE_AC2, STATE_FAULT };
State currentState = STATE_IDLE;

struct BatteryData {
  uint16_t voltage_mv;
  int8_t   soc_percent;
  bool     low_warning;
};
BatteryData battery;

volatile bool canInterrupt  = false;  // IRAM_ATTR ISR Flag
volatile bool pcaInterrupt  = false;  // IRAM_ATTR ISR Flag

// ─────────────────────────────────────────────
//  INTERRUPT SERVICE ROUTINES (ISR)
//  Kurz halten — nur Flag setzen
// ─────────────────────────────────────────────
void IRAM_ATTR onCanInterrupt()  { canInterrupt  = true; }
void IRAM_ATTR onPcaInterrupt()  { pcaInterrupt  = true; }

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // M5 Display initialisieren
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setTextSize(2);
  M5.Display.println("ACOS Init...");

  // I2C starten
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // SPI starten
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

  // PCA9555 initialisieren
  setupExpander();

  // MCP2515 initialisieren
  setupCAN();

  // ADC konfigurieren
  analogSetAttenuation(ADC_11db);  // 0–3.3V Bereich
  analogReadResolution(ADC_RESOLUTION);

  // Interrupts aktivieren
  pinMode(PIN_CAN_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CAN_INT), onCanInterrupt, FALLING);

  pinMode(PIN_PCA_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_PCA_INT), onPcaInterrupt, FALLING);

  M5.Display.println("Ready!");
  Serial.println("[ACOS] Init abgeschlossen.");
}

// ─────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────
void loop() {
  static uint32_t lastLoop = 0;
  if (millis() - lastLoop < LOOP_INTERVAL_MS) return;
  lastLoop = millis();

  M5.update();  // Touch + Buttons aktualisieren

  // Interrupt-Handler im Loop-Kontext aufrufen
  if (canInterrupt)  { canInterrupt  = false; handleCANReceive(); }
  if (pcaInterrupt)  { pcaInterrupt  = false; handlePCAInputs();  }

  // Periodische Aufgaben
  readBattery();
  runStateMachine();
  updateDisplay();
  sendCANStatus();
}

// ═══════════════════════════════════════════════════════════════
//  PCA9555 SETUP & HANDLING
// ═══════════════════════════════════════════════════════════════

void setupExpander() {
  if (!expander.begin()) {
    Serial.println("[PCA] FEHLER: PCA9555 nicht erreichbar!");
    return;
  }

  // Port 0 (Pins 0-7) = Eingänge
  for (int i = 0; i <= 7; i++) {
    expander.pinMode(i, INPUT);   // PCA9555 hat interne Pull-Ups
  }

  // Port 1 (Pins 8-15) = Ausgänge, initial LOW
  for (int i = 8; i <= 15; i++) {
    expander.pinMode(i, OUTPUT);
    expander.digitalWrite(i, LOW);
  }

  Serial.println("[PCA] PCA9555 initialisiert.");
}

void handlePCAInputs() {
  // pinStates() liest alle 16 Pins auf einmal → effizient
  uint16_t states = expander.pinStates();

  // Eingänge auslesen (Active LOW → invertiert)
  bool sw1_active = READ_ACTIVE_LOW(bitRead(states, EXPPIN_SWITCH_AC1));
  bool sw2_active = READ_ACTIVE_LOW(bitRead(states, EXPPIN_SWITCH_AC2));
  bool fault1     = READ_ACTIVE_LOW(bitRead(states, EXPPIN_FAULT_AC1));
  bool fault2     = READ_ACTIVE_LOW(bitRead(states, EXPPIN_FAULT_AC2));
  bool overtemp   = READ_ACTIVE_LOW(bitRead(states, EXPPIN_OVERTEMP));
  bool emergency  = READ_ACTIVE_LOW(bitRead(states, EXPPIN_EMERGENCY));

  Serial.printf("[PCA] SW1=%d SW2=%d F1=%d F2=%d OT=%d EMG=%d\n",
                sw1_active, sw2_active, fault1, fault2, overtemp, emergency);

  // Zustandslogik
  if (emergency || overtemp || (fault1 && fault2)) {
    currentState = STATE_FAULT;
  } else if (sw1_active && !fault1) {
    currentState = STATE_AC1;
  } else if (sw2_active && !fault2) {
    currentState = STATE_AC2;
  }
}

// ═══════════════════════════════════════════════════════════════
//  ZUSTANDSMASCHINE
// ═══════════════════════════════════════════════════════════════

void runStateMachine() {
  static State lastState = STATE_IDLE;
  if (currentState == lastState) return;  // kein Zustandswechsel

  // Beim Wechsel: Beide Relais erst AUS, dann warten, dann EIN
  EXP_WRITE_RELAY(expander, EXPPIN_RELAY_AC1, false);
  EXP_WRITE_RELAY(expander, EXPPIN_RELAY_AC2, false);
  delay(RELAY_SWITCH_DELAY_MS);

  switch (currentState) {
    case STATE_AC1:
      EXP_WRITE_RELAY(expander, EXPPIN_RELAY_AC1, true);
      EXP_WRITE_LED(expander, EXPPIN_LED_OK, true);
      EXP_WRITE_LED(expander, EXPPIN_LED_WARN, false);
      EXP_WRITE_LED(expander, EXPPIN_LED_FAULT, false);
      Serial.println("[STATE] → AC1 aktiv");
      break;

    case STATE_AC2:
      EXP_WRITE_RELAY(expander, EXPPIN_RELAY_AC2, true);
      EXP_WRITE_LED(expander, EXPPIN_LED_WARN, true);
      EXP_WRITE_LED(expander, EXPPIN_LED_OK, false);
      EXP_WRITE_LED(expander, EXPPIN_LED_FAULT, false);
      Serial.println("[STATE] → AC2 aktiv");
      break;

    case STATE_FAULT:
      EXP_WRITE_LED(expander, EXPPIN_LED_FAULT, true);
      EXP_WRITE_LED(expander, EXPPIN_LED_OK, false);
      EXP_WRITE_LED(expander, EXPPIN_LED_WARN, false);
      expander.digitalWrite(EXPPIN_BUZZER, HIGH);
      Serial.println("[STATE] → FAULT");
      break;

    case STATE_IDLE:
    default:
      EXP_WRITE_LED(expander, EXPPIN_LED_OK, false);
      EXP_WRITE_LED(expander, EXPPIN_LED_WARN, false);
      EXP_WRITE_LED(expander, EXPPIN_LED_FAULT, false);
      expander.digitalWrite(EXPPIN_BUZZER, LOW);
      Serial.println("[STATE] → IDLE");
      break;
  }

  lastState = currentState;
}

// ═══════════════════════════════════════════════════════════════
//  ADC — BATTERIE-MESSUNG & SOC-BERECHNUNG
// ═══════════════════════════════════════════════════════════════

void readBattery() {
  // Mehrere Samples mitteln → Rauschen reduzieren
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogRead(PIN_BATT_VOLTAGE);
  uint16_t raw = sum / 16;

  // ADC-Wert → mV → reale Spannung (mit Spannungsteiler)
  float adc_mv  = (raw * ADC_VREF_MV) / ((1 << ADC_RESOLUTION) - 1.0f);
  battery.voltage_mv = (uint16_t)(adc_mv * BATT_VOLTAGE_DIVIDER);

  // SOC berechnen (lineare Näherung)
  battery.soc_percent = map(
    constrain(battery.voltage_mv, BATT_EMPTY_MV, BATT_FULL_MV),
    BATT_EMPTY_MV, BATT_FULL_MV, 0, 100
  );

  battery.low_warning = (battery.soc_percent <= BATT_WARN_SOC);
}

// ═══════════════════════════════════════════════════════════════
//  CAN BUS SETUP & KOMMUNIKATION (autowp/arduino-mcp2515)
// ═══════════════════════════════════════════════════════════════

void setupCAN() {
  mcp2515.reset();

  if (mcp2515.setBitrate(CAN_BITRATE, CAN_CLOCK_FREQ) != MCP2515::ERROR_OK) {
    Serial.println("[CAN] FEHLER: Bitrate setzen fehlgeschlagen!");
    return;
  }

  // Optional: Filter setzen (nur bestimmte IDs empfangen)
  // mcp2515.setFilterMask(MCP2515::MASK0, false, 0x7FF);  // Standard 11-bit Maske
  // mcp2515.setFilter(MCP2515::RXF0, false, CAN_ID_SETPOINT);

  mcp2515.setNormalMode();  // Normaler Betrieb (senden + empfangen)
  Serial.println("[CAN] MCP2515 initialisiert.");
}

// CAN Nachricht empfangen (wird im Loop bei Interrupt aufgerufen)
void handleCANReceive() {
  struct can_frame frame;

  while (mcp2515.readMessage(&frame) == MCP2515::ERROR_OK) {
    Serial.printf("[CAN] RX ID=0x%03X DLC=%d  ", frame.can_id, frame.can_dlc);
    for (int i = 0; i < frame.can_dlc; i++) {
      Serial.printf("%02X ", frame.data[i]);
    }
    Serial.println();

    // Steuerbefehl verarbeiten
    if (frame.can_id == CAN_ID_SETPOINT && frame.can_dlc >= 1) {
      uint8_t cmd = frame.data[0];
      if      (cmd == 0x01) currentState = STATE_AC1;
      else if (cmd == 0x02) currentState = STATE_AC2;
      else if (cmd == 0x00) currentState = STATE_IDLE;
    }
  }
}

// CAN Status senden (periodisch)
void sendCANStatus() {
  static uint32_t lastSend = 0;
  if (millis() - lastSend < 1000) return;  // alle 1 Sekunde
  lastSend = millis();

  // Zustandsmeldung
  struct can_frame statusFrame;
  statusFrame.can_id  = CAN_ID_STATUS;
  statusFrame.can_dlc = 2;
  statusFrame.data[0] = (uint8_t)currentState;
  statusFrame.data[1] = battery.soc_percent;

  if (mcp2515.sendMessage(&statusFrame) != MCP2515::ERROR_OK) {
    Serial.println("[CAN] TX Fehler: Status");
  }

  // Batterie-Werte senden
  struct can_frame battFrame;
  battFrame.can_id  = CAN_ID_BATTERY;
  battFrame.can_dlc = 3;
  battFrame.data[0] = highByte(battery.voltage_mv);
  battFrame.data[1] = lowByte(battery.voltage_mv);
  battFrame.data[2] = battery.soc_percent;

  mcp2515.sendMessage(&battFrame);
}

// ═══════════════════════════════════════════════════════════════
//  DISPLAY (M5Unified)
// ═══════════════════════════════════════════════════════════════

void updateDisplay() {
  static uint32_t lastDraw = 0;
  if (millis() - lastDraw < 500) return;  // nur alle 500ms neu zeichnen
  lastDraw = millis();

  M5.Display.setCursor(0, 0);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE);

  // Zustand
  M5.Display.setTextSize(2);
  M5.Display.print("Zustand: ");
  switch (currentState) {
    case STATE_AC1:   M5.Display.setTextColor(GREEN);  M5.Display.println("AC1"); break;
    case STATE_AC2:   M5.Display.setTextColor(YELLOW); M5.Display.println("AC2"); break;
    case STATE_FAULT: M5.Display.setTextColor(RED);    M5.Display.println("FEHLER"); break;
    default:          M5.Display.setTextColor(CYAN);   M5.Display.println("IDLE"); break;
  }

  // Batterie
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(1);
  M5.Display.printf("Bat: %d mV  SOC: %d%%\n", battery.voltage_mv, battery.soc_percent);
  if (battery.low_warning) {
    M5.Display.setTextColor(RED);
    M5.Display.println("! BATTERIE SCHWACH !");
  }
}
```

---

## Zusammenfassung: Was sich vereinfacht hat

| Vorher (C++/PlatformIO)          | Jetzt (Arduino)                          |
|----------------------------------|------------------------------------------|
| `pca9555.h/.cpp` (eigener Treiber) | `PCA9555` Library → `pinMode()`, `digitalRead()`, `digitalWrite()` |
| `hardware.h/.cpp` (HAL-Schicht)  | Entfällt — Libraries übernehmen das      |
| `state_machine.h/.cpp`           | Einfache `switch/case` in `runStateMachine()` |
| `adc_reader.h/.cpp`              | Direkt `analogRead()` + Formel           |
| `main.cpp` + 6 Header-Dateien    | `ACOS.ino` + `config.h`                  |
| `platformio.ini`                 | Arduino IDE Library Manager              |

## Library-Installation

```
Arduino IDE → Sketch → Bibliothek einbinden → Bibliotheken verwalten...
Suche: "PCA9555"    → installieren (by Nico Verduin)
Suche: "MCP2515"    → installieren (by autowp)
Suche: "M5Unified"  → installieren (by M5Stack)
```

> **Hinweis MCP2515 + ESP32-S3:** Das MCP2515-Modul braucht 5V Versorgung (nicht 3.3V!), 
> und die SPI-Leitungen sollten mit einem Level-Shifter oder über ein 3.3V-kompatibles Modul
> (z.B. mit TJA1050 statt MCP2551) verbunden werden.

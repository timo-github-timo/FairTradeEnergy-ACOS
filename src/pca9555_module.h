#pragma once
// ================================================================
//  pca9555_module.h  —  IC9 PCA9555PW I/O-Expander
//  Bibliothek : nicoverduin/PCA9555  (Arduino Library Manager)
//  Zielplattform: ESP32-S3 (M5CoreS3SE)
//
//  Port 0 (IO0_0–IO0_7) — Ausgänge / gemischte Richtungen
//  Port 1 (IO1_0–IO1_7) — Eingänge / externe I/Os
//
//  Interrupt-Betrieb (ESP32-S3-sicher):
//    Die ISR (IRAM_ATTR) gibt nur ein FreeRTOS-Semaphor —
//    kein I²C in der ISR. Ein dedizierter FreeRTOS-Task wartet
//    auf das Semaphor, liest dann beide Ports und ruft den
//    Callback auf. I²C läuft damit sicher außerhalb der ISR.
//
//  Abhängigkeiten: config.h, <clsPCA9555.h>
// ================================================================

#include "config.h"
#include <clsPCA9555.h>

// ----------------------------------------------------------------
//  I²C-Adresse  (A0=1, A1=1, A2=1 → 0x27; anpassen falls nötig)
// ----------------------------------------------------------------
#define PCA9555_I2C_ADDR    0x27

// ----------------------------------------------------------------
//  Pin-Mapping  (Bibliothek: 0–7 = Port 0, 8–15 = Port 1)
// ----------------------------------------------------------------

// Port 0 — Ausgänge
#define PIN_GRID_ON     ED0     // IO0_0  — Netz-Relais (REL1 Latching Driver)
#define PIN_ISLE_ON     ED1     // IO0_1  — Insel-Relais (REL2 Latching Driver)
#define PIN_GI_SEL      ED2     // IO0_2  — Grid/Island Umschaltung (REL4 + IC3/IC4)
#define PIN_ASEL1       ED4     // IO0_4  — Analogkanal-Bit 0 (IC14 + IC11)
#define PIN_ASEL2       ED5     // IO0_5  — Analogkanal-Bit 1 (IC14 + IC12)

// Port 0 — Eingang
#define PIN_NTC_HOT     ED3     // IO0_3  — NTC Übertemperatur (Komparator IC7 MCP6541, active-HIGH)

// Port 1 — Eingänge (Open-Collector, active-LOW)
#define PIN_nVG2        ED8     // IO1_0  — !VG2: Spannung L_G2 erkannt (OC1)
#define PIN_nVI2        ED9     // IO1_1  — !VI2: Spannung L_I2 erkannt (OC2)
#define PIN_nVGI        ED10    // IO1_2  — !VGI: Spannung L_GI erkannt (OC3)

// Port 1 — Externe I/Os (über 100Ω nach aussen)
#define PIN_IO_0        ED13    // IO1_5  — Externer I/O 0 (R94)
#define PIN_IO_1        ED14    // IO1_6  — Externer I/O 1 (R95)
#define PIN_IO_2        ED15    // IO1_7  — Externer I/O 2 (R96)

// ----------------------------------------------------------------
//  Aktueller Zustand aller Eingänge
//  active-LOW Signale (!VG2, !VI2, !VGI) sind bereits invertiert:
//  true = Spannung/Zustand erkannt.
// ----------------------------------------------------------------
struct Pca9555Inputs {
    bool ntc_hot;   // true = NTC Übertemperatur (active-HIGH)
    bool vg2;       // true = Spannung L_G2 erkannt  (!VG2 invertiert)
    bool vi2;       // true = Spannung L_I2 erkannt  (!VI2 invertiert)
    bool vgi;       // true = Spannung L_GI erkannt  (!VGI invertiert)
    bool io[3];     // io[0..2] = externe I/O-Pins (IO_0, IO_1, IO_2)
};

// ----------------------------------------------------------------
//  Callback-Typ
//  Wird aus dem IRQ-Task aufgerufen (normaler Task-Kontext,
//  nicht ISR) — Serial.print(), Mutex, etc. sind erlaubt.
// ----------------------------------------------------------------
typedef void (*Pca9555Callback)(const Pca9555Inputs& inputs);

// ----------------------------------------------------------------
//  Öffentliche Funktionen
// ----------------------------------------------------------------

// Einmalig in setup() aufrufen.
// irqPin  : Arduino-Pin der mit INT von IC9 verbunden ist (G1).
// callback: Funktion die bei jeder Eingangsänderung aufgerufen wird.
// Gibt true zurück wenn IC9 erreichbar ist.
bool pca9555_init(uint8_t irqPin, Pca9555Callback callback);

// true wenn IC9 erfolgreich initialisiert wurde (für Debug-Ausgaben).
bool pca9555_is_ok();

// --- Ausgänge (Port 0) ------------------------------------------

void pca9555_set_grid_on(bool on);
void pca9555_set_isle_on(bool on);
void pca9555_set_gi_sel(bool grid);     // true = Grid, false = Insel
void pca9555_set_asel(uint8_t channel); // 0–3

// --- Eingänge — direkter Lesezugriff via I²C --------------------
// Für einmalige Abfragen außerhalb des Interrupt-Pfads (z.B. setup).

bool pca9555_get_ntc_hot();
bool pca9555_get_vg2();
bool pca9555_get_vi2();
bool pca9555_get_vgi();
bool pca9555_get_io(uint8_t n);   // n = 0, 1 oder 2

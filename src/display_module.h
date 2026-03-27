#pragma once
// ================================================================
//  display_module.h  —  Display & Touch (M5CoreS3SE, 320×240)
//  Bibliothek : M5Unified  (Arduino Library Manager)
//
//  Layout:
//    ┌─────────────────────────────────────────┐
//    │  U_grid [V] │ F_grid [Hz] │ SoC [%] │ Bat [V] │  ← Tiles  H=48
//    ├─────────────────────────────────────────┤
//    │                                         │         ← Karten  H=162
//    │   AUTO-View  oder  MANUAL-View          │
//    ├─────────────────────────────────────────┤
//    │  Status:                                │         ← Status  H=26
//    └─────────────────────────────────────────┘
//
//  Ablauf:
//    setup()  → display_init()
//    loop()   → display_update(data)   (zeichnet bei Änderung neu)
//             → display_handle_touch() (gibt DispEvent zurück)
//
//  Abhängigkeiten: config.h, <M5Unified.h>
// ================================================================

#include "config.h"
#include <M5Unified.h>

// ----------------------------------------------------------------
//  Messwerte für die Anzeige
// ----------------------------------------------------------------
struct DisplayData {
    float   u_grid;     // Netzspannung      [V]
    float   f_grid;     // Netzfrequenz      [Hz]
    int8_t  soc;        // SoC               [%]
    float   batt_v;     // Batteriespannung  [V]
    String  status;     // Statuszeile unten
};

// ----------------------------------------------------------------
//  Ereignisse — Rückgabe von display_handle_touch()
//  Caller wertet aus und reagiert (keine Display-Logik im Caller).
// ----------------------------------------------------------------
enum DispEvent {
    DISP_EVT_NONE,          // kein Touch / kein Ereignis
    DISP_EVT_TO_MANUAL,     // AUTO-View: "Manual"-Karte angetippt
    DISP_EVT_TO_AUTO,       // MANUAL-View: Zurück-Touch (Status-Bar)
    DISP_EVT_MODE_ISLAND,   // MANUAL-View: "Island" gewählt
    DISP_EVT_MODE_OFF,      // MANUAL-View: "OFF" gewählt
    DISP_EVT_MODE_GRID,     // MANUAL-View: "Grid" gewählt
};

// ----------------------------------------------------------------
//  Öffentliche Funktionen
// ----------------------------------------------------------------

// Einmalig in setup(): Display initialisieren, Startscreen zeigen.
void display_init();

// Im loop(): Bildschirm aktualisieren.
// Zeichnet nur neu wenn sich data geändert hat oder die View wechselt.
void display_update(const DisplayData& data);

// Im loop() nach M5.update(): Touch auswerten.
// Gibt DispEvent zurück; DISP_EVT_NONE wenn kein relevanter Touch.
DispEvent display_handle_touch();

#pragma once
// ================================================================
//  state_machine.h  —  ACOS Zustandsmaschine
//
//  Zustände:  OFF, ISLAND, GRID, TRANSITION_TO_GRID,
//             TRANSITION_TO_ISLAND, ERROR
//
//  Globale Sicherheitsregeln (übersteuern ALLES):
//    1. NTC Übertemperatur  → sofort ERROR
//    2. Netz-Überspannung   → sofort ERROR
//
//  Umschaltsequenz (Break-before-Make):
//    Beide Relais AUS → warten auf Last spannungsfrei (!VLOAD HIGH)
//    → GI_SEL setzen → 50ms warten → neues Relais EIN
//
//  Abhängigkeiten: config.h, pca9555_module.h
// ================================================================

#include "config.h"

// Einmalig in setup() aufrufen (nach pca9555_init).
// Setzt Safe State: alle Relais AUS, Zustand = OFF.
void sm_init();

// Haupt-Tick — alle 100ms aus loop() aufrufen.
// readings : aktuelle Sensor-Messwerte
// mode     : aktueller Betriebsmodus (Touch / CAN)
void sm_tick(const SensorReadings& readings, OperatingMode mode);

// Aktuellen Zustand abfragen.
AcosState sm_get_state();

// Fehlermeldung (nur gültig wenn sm_get_state() == STATE_ERROR).
const char* sm_get_error_msg();

// Fehler quittieren: ERROR → OFF.
// Wirkungslos wenn nicht im ERROR-Zustand.
void sm_clear_error();

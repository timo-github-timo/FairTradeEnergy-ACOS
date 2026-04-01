// ================================================================
//  state_machine.cpp  —  ACOS Zustandsmaschine
// ================================================================

#include "state_machine.h"
#include "pca9555_module.h"
#include <Arduino.h>
#include <string.h>

static AcosState _state        = STATE_OFF;
static uint32_t  _trans_start  = 0;
static char      _error_msg[48] = "";

// ----------------------------------------------------------------
//  Beide Relais sicher ausschalten
// ----------------------------------------------------------------
static void relays_off() {
    pca9555_set_grid_on(false);
    pca9555_set_isle_on(false);
}

// ----------------------------------------------------------------
//  Fehler setzen + ERROR-Zustand einleiten
// ----------------------------------------------------------------
static void set_error(const char* msg) {
    relays_off();
    strncpy(_error_msg, msg, sizeof(_error_msg) - 1);
    _error_msg[sizeof(_error_msg) - 1] = '\0';
    _state = STATE_ERROR;
}

// ----------------------------------------------------------------
//  Zustand wechseln + Entry-Aktionen ausführen
//
//  HINWEIS zu pca9555_set_gi_sel():
//    Laut Schaltplan: GI_SEL=LOW (0) = Grid-Pfad, GI_SEL=HIGH (1) = Island-Pfad.
//    Die Implementierung setzt: false → LOW (Grid), true → HIGH (Island).
//    Der Kommentar im Header (true=Grid) ist ein Fehler im Quellcode —
//    die Hardware-Zuordnung hier ist elektrisch korrekt.
// ----------------------------------------------------------------
static void enter_state(AcosState new_state) {
    _state = new_state;

    switch (new_state) {

        case STATE_OFF:
            relays_off();
            break;

        case STATE_TRANSITION_TO_GRID:
        case STATE_TRANSITION_TO_ISLAND:
            // Break-before-Make: erst beide Relais AUS, dann auf VLOAD warten
            relays_off();
            _trans_start = millis();
            break;

        case STATE_GRID:
            // GI_SEL auf Grid-Pfad, 50ms Einschwingzeit, dann Grid-Relais EIN
            // delay() ist hier akzeptabel: passiert nur bei Umschaltung (selten)
            pca9555_set_gi_sel(false);  // false = LOW = Grid-Pfad (laut Schaltplan)
            delay(50);
            pca9555_set_isle_on(false);
            pca9555_set_grid_on(true);
            break;

        case STATE_ISLAND:
            pca9555_set_gi_sel(true);   // true = HIGH = Island-Pfad (laut Schaltplan)
            delay(50);
            pca9555_set_grid_on(false);
            pca9555_set_isle_on(true);
            break;

        case STATE_ERROR:
            relays_off();
            break;
    }
}

// ----------------------------------------------------------------
//  Public API
// ----------------------------------------------------------------

void sm_init() {
    _state = STATE_OFF;
    relays_off();
    _error_msg[0] = '\0';
}

AcosState   sm_get_state()     { return _state;     }
const char* sm_get_error_msg() { return _error_msg; }

void sm_clear_error() {
    if (_state == STATE_ERROR) {
        _error_msg[0] = '\0';
        enter_state(STATE_OFF);
    }
}

// ----------------------------------------------------------------
//  Haupt-Tick  (alle 100ms)
// ----------------------------------------------------------------
void sm_tick(const SensorReadings& s, OperatingMode mode) {

    // ---- Globale Sicherheitsregeln — übersteuern ALLES -----------
    // Laufen vor jedem Zustandshandler; erzwingen sofortigen ERROR.
    if (s.ntc_hot && _state != STATE_ERROR) {
        set_error("Uebertemperatur");
        return;
    }
    if (s.grid_ov && _state != STATE_ERROR) {
        set_error("Netz-Ueberspannung");
        return;
    }

    // ---- Zustandshandler ----------------------------------------
    switch (_state) {

        // ---- OFF ------------------------------------------------
        case STATE_OFF:
            if (mode == MODE_AUTO) {
                if (s.soc >= THR_SOC_MIN && !s.grid_ok && s.v_load < THR_SWITCH_V) {
                    // Batterie geladen, kein Netz → Inselbetrieb
                    enter_state(STATE_TRANSITION_TO_ISLAND);
                } else if (s.soc < THR_SOC_MAX && s.grid_ok && s.v_load < THR_SWITCH_V) {
                    // Netz verfügbar, Batterie nicht voll → Netzbetrieb
                    enter_state(STATE_TRANSITION_TO_GRID);
                }
            } else if (mode == MODE_HAND_ISLAND) {
                if (s.soc > THR_SOC_EMERGENCY && s.v_load < THR_SWITCH_V) {
                    enter_state(STATE_TRANSITION_TO_ISLAND);
                }
            } else if (mode == MODE_HAND_GRID) {
                if (s.grid_ok && s.v_load < THR_SWITCH_V) {
                    enter_state(STATE_TRANSITION_TO_GRID);
                }
            }
            // MODE_HAND_OFF: bleibt in OFF
            break;

        // ---- ISLAND ---------------------------------------------
        case STATE_ISLAND:
            // Sicherheit: Batterie kritisch leer → Last abschalten
            if (s.soc < THR_SOC_EMERGENCY) {
                enter_state(STATE_OFF);
                break;
            }
            if (mode == MODE_AUTO) {
                if (s.soc < THR_SOC_MIN && s.grid_ok) {
                    // Batterie wird knapp, Netz verfügbar → umschalten
                    enter_state(STATE_TRANSITION_TO_GRID);
                }
            } else if (mode == MODE_HAND_OFF) {
                enter_state(STATE_OFF);
            } else if (mode == MODE_HAND_GRID && s.grid_ok) {
                enter_state(STATE_TRANSITION_TO_GRID);
            }
            break;

        // ---- GRID -----------------------------------------------
        case STATE_GRID:
            if (mode == MODE_AUTO) {
                if (!s.grid_ok && s.soc >= THR_SOC_MIN) {
                    // Netzausfall, Batterie hat genug Ladung → Insel
                    enter_state(STATE_TRANSITION_TO_ISLAND);
                } else if (!s.grid_ok && s.soc < THR_SOC_MIN) {
                    // Netzausfall, Batterie zu leer → Last abschalten
                    enter_state(STATE_OFF);
                } else if (s.soc > THR_SOC_MAX) {
                    // Batterie voll → Inselbetrieb bevorzugen
                    enter_state(STATE_TRANSITION_TO_ISLAND);
                }
            } else if (mode == MODE_HAND_OFF) {
                enter_state(STATE_OFF);
            } else if (mode == MODE_HAND_ISLAND && s.soc > THR_SOC_EMERGENCY) {
                enter_state(STATE_TRANSITION_TO_ISLAND);
            }
            break;

        // ---- TRANSITION TO GRID ---------------------------------
        case STATE_TRANSITION_TO_GRID:
            if (!s.load_on) {
                // Last spannungsfrei — Grid-Relais einschalten
                enter_state(STATE_GRID);
            } else if (millis() - _trans_start >= TIMEOUT_RELAY_MS) {
                set_error("Relais klemmt (->Netz)");
            }
            break;

        // ---- TRANSITION TO ISLAND -------------------------------
        case STATE_TRANSITION_TO_ISLAND:
            if (!s.load_on) {
                // Last spannungsfrei — Island-Relais einschalten
                enter_state(STATE_ISLAND);
            } else if (millis() - _trans_start >= TIMEOUT_RELAY_MS) {
                set_error("Relais klemmt (->Insel)");
            }
            break;

        // ---- ERROR ----------------------------------------------
        case STATE_ERROR:
            // Alle Relais AUS — warte auf sm_clear_error() von außen
            break;
    }
}

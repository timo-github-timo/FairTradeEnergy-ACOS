// ================================================================
//  ACOS.cpp  —  Hauptprogramm (setup + loop)
//
//  Zyklus: 100ms
//    1. Sensoren lesen (ADC + GPIO Optokoppler + PCA9555)
//    2. Zustandsmaschine ticken
//    3. Touch auswerten → Modus ändern
//    4. CAN empfangen  → Modus / Fehler-Quittierung
//    5. CAN Status senden (1Hz)
//    6. Display aktualisieren
//    7. Serial-Debug (2s Intervall)
//    8. Serial-Befehle auswerten
//    9. Loop-Timing einhalten
// ================================================================

#include <Arduino.h>
#include <M5Unified.h>
#include <Wire.h>
#include <SPI.h>

#include "config.h"
#include "pca9555_module.h"
#include "adc_reader.h"
#include "state_machine.h"
#include "can_module.h"
#include "display_module.h"

// ----------------------------------------------------------------
//  Globaler Betriebsmodus
//  Touch und CAN schreiben, sm_tick() liest.
// ----------------------------------------------------------------
static OperatingMode g_mode = MODE_AUTO;

// CAN TX standardmäßig deaktiviert (Beobachter-Modus ohne Platine).
// Serial-Befehl 't' schaltet um. CAN RX läuft immer wenn MCP2515 ok.
static bool g_can_tx_enabled = false;

// Letzter Zustand der Optokoppler-Pins für Debug-Dump
static bool g_can_init_ok = false;

// ----------------------------------------------------------------
//  PCA9555 Interrupt-Callback
//  Läuft im IRQ-Task (Core 1, normaler Task-Kontext).
//  Nur Flag + Struct setzen — keine langen Operationen.
// ----------------------------------------------------------------
static volatile bool g_io_updated = false;
static Pca9555Inputs g_inputs     = {};

static void on_io_change(const Pca9555Inputs& in) {
    g_inputs     = in;
    g_io_updated = true;
}

// ----------------------------------------------------------------
//  Optokoppler-Eingänge lesen  (GPIO, active-LOW)
// ----------------------------------------------------------------
static inline bool read_grid_ok() { return digitalRead(PIN_nVGRID)    == LOW; }
static inline bool read_grid_ov() { return digitalRead(PIN_nVGRID_OV) == LOW; }
static inline bool read_load_on() { return digitalRead(PIN_nVLOAD)    == LOW; }
static inline bool read_isle_ok() { return digitalRead(PIN_nVISLE)    == LOW; }

// ----------------------------------------------------------------
//  Alle Sensoren lesen → SensorReadings befüllen
// ----------------------------------------------------------------
static void read_sensors(SensorReadings& s) {
    adc_read_all(s.v_grid, s.v_batt, s.v_load);
    s.soc     = adc_calc_soc(s.v_batt);
    s.grid_ok = read_grid_ok();
    s.grid_ov = read_grid_ov();
    s.load_on = read_load_on();
    s.isle_ok = read_isle_ok();
    s.ntc_hot = g_inputs.ntc_hot;  // aus letztem PCA9555-Interrupt
}

// ----------------------------------------------------------------
//  Klartext-Namen für Serial und Display
// ----------------------------------------------------------------
static const char* const STATE_NAMES[] = {
    "AUS", "Insel", "Netz", "->Netz...", "->Insel...", "FEHLER"
};
static const char* const MODE_NAMES[] = {
    "Auto", "Hand:Netz", "Hand:Insel", "Hand:Aus"
};

// ----------------------------------------------------------------
//  Setup
// ----------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    // I²C mit expliziten Pins (ST1 Pin 13/14: SCL=G11, SDA=G12)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // Optokoppler-Eingänge (active-LOW, open-collector)
    pinMode(PIN_nVGRID,    INPUT_PULLUP);
    pinMode(PIN_nVGRID_OV, INPUT_PULLUP);
    pinMode(PIN_nVLOAD,    INPUT_PULLUP);
    pinMode(PIN_nVISLE,    INPUT_PULLUP);

    // PCA9555 initialisieren — Safe State wird intern gesetzt
    if (!pca9555_init(PIN_PCA9555_INT, on_io_change)) {
        Serial.println("[FEHLER] IC9 PCA9555 nicht erreichbar!");
    } else {
        Serial.println("[OK] IC9 PCA9555 initialisiert.");
    }

    // ADC initialisieren (12-Bit, 11dB)
    adc_init();
    Serial.println("[OK] ADC initialisiert.");

    // Zustandsmaschine: Safe State (alle Relais AUS, Zustand OFF)
    sm_init();
    Serial.println("[OK] Zustandsmaschine initialisiert.");

    // CAN-Bus
    g_can_init_ok = can_init();
    if (!g_can_init_ok) {
        Serial.println("[WARNUNG] CAN MCP2515 nicht erreichbar — CAN deaktiviert.");
    } else {
        Serial.println("[OK] CAN initialisiert.");
        Serial.println("[INFO] CAN TX deaktiviert (Beobachter-Modus). 't' = umschalten.");
    }

    // Display
    display_init();
    Serial.println("[OK] Display initialisiert.");

    Serial.println("[ACOS] Bereit.");
    Serial.println("  Modus:  a=Auto  g=Hand:Netz  i=Hand:Insel  o=Hand:Aus  c=Fehler_quit");
    Serial.println("  CAN TX: t=umschalten (aktuell: AUS)");
    Serial.println("  Debug:  d=Sofort-Dump");
}

// ----------------------------------------------------------------
//  Loop — 100ms Zykluszeit
// ----------------------------------------------------------------
static DisplayData g_disp = {};

void loop() {
    const uint32_t CYCLE_MS   = 100;
    const uint32_t SERIAL_MS  = 2000;
    const uint32_t CAN_TX_MS  = 1000;

    static uint32_t last_serial = 0;
    static uint32_t last_can_tx = 0;
    uint32_t        cycle_start = millis();

    M5.update();

    // 1. Sensoren lesen
    SensorReadings s = {};
    read_sensors(s);

    // Für Serial-Debug: PCA9555-Änderungen protokollieren
    if (g_io_updated) {
        g_io_updated = false;
        Serial.printf("[PCA9555] NTC=%d  VG2=%d  VI2=%d  VGI=%d\n",
                      g_inputs.ntc_hot, g_inputs.vg2, g_inputs.vi2, g_inputs.vgi);
    }

    // 2. Zustandsmaschine ticken
    sm_tick(s, g_mode);
    AcosState state = sm_get_state();

    // 3. Touch auswerten → Modus ändern
    DispEvent evt = display_handle_touch();
    switch (evt) {
        case DISP_EVT_TO_MANUAL:                               break;  // nur View-Wechsel
        case DISP_EVT_TO_AUTO:   g_mode = MODE_AUTO;          break;
        case DISP_EVT_MODE_ISLAND: g_mode = MODE_HAND_ISLAND; break;
        case DISP_EVT_MODE_GRID:   g_mode = MODE_HAND_GRID;   break;
        case DISP_EVT_MODE_OFF:    g_mode = MODE_HAND_OFF;    break;
        default: break;
    }

    // Fehler-Quittierung: beliebiger Touch wenn im ERROR-Zustand
    if (state == STATE_ERROR) {
        auto tp = M5.Touch.getDetail();
        if (tp.wasPressed()) {
            sm_clear_error();
            g_mode = MODE_AUTO;
            state  = sm_get_state();
        }
    }

    // 4. CAN empfangen (Polling)
    bool can_err_clear   = false;
    OperatingMode can_mode = can_receive(can_err_clear);
    if (can_mode != MODE_AUTO) {
        g_mode = can_mode;   // CAN-Befehl überschreibt Touch-Modus
    }
    if (can_err_clear) {
        sm_clear_error();
        g_mode = MODE_AUTO;
        state  = sm_get_state();
    }

    // 5. CAN Status senden (1Hz, nur wenn TX aktiv)
    if (g_can_tx_enabled && millis() - last_can_tx >= CAN_TX_MS) {
        last_can_tx = millis();
        uint8_t flags = (uint8_t)(
            (s.grid_ok ? 0x01 : 0) |
            (s.grid_ov ? 0x02 : 0) |
            (s.load_on ? 0x04 : 0) |
            (s.ntc_hot ? 0x08 : 0)
        );
        can_send_status(state, g_mode, s.soc, s.v_grid, s.v_batt, flags);
    }

    // 6. Display aktualisieren
    char status_buf[48];
    if (state == STATE_ERROR) {
        snprintf(status_buf, sizeof(status_buf), "ERR: %s", sm_get_error_msg());
    } else {
        snprintf(status_buf, sizeof(status_buf), "%s | %s",
                 STATE_NAMES[state], MODE_NAMES[g_mode]);
    }
    g_disp.status = status_buf;
    g_disp.u_grid = s.v_grid;
    g_disp.f_grid = 50.0f;      // TODO: Frequenzmessung noch nicht implementiert
    g_disp.soc    = s.soc;
    g_disp.batt_v = s.v_batt;

    display_update(g_disp);

    // 7. Serial-Debug (alle 2s)
    if (millis() - last_serial >= SERIAL_MS) {
        last_serial = millis();
        float raw_grid, raw_batt, raw_load;
        adc_read_raw_voltages(raw_grid, raw_batt, raw_load);
        Serial.printf("[ACOS] t=%lums | %s | %s | SOC=%d%%"
                      " | VGrid=%.1fV(roh=%.3fV) | VBatt=%.1fV(roh=%.3fV) | VLoad=%.1fV(roh=%.3fV)"
                      " | GPIO: nVGRID=%d nVGRID_OV=%d nVLOAD=%d nVISLE=%d"
                      " | PCA: NTC=%d VG2=%d VI2=%d VGI=%d"
                      " | CAN-TX=%s\n",
                      millis(),
                      STATE_NAMES[state], MODE_NAMES[g_mode], s.soc,
                      s.v_grid, raw_grid, s.v_batt, raw_batt, s.v_load, raw_load,
                      digitalRead(PIN_nVGRID), digitalRead(PIN_nVGRID_OV),
                      digitalRead(PIN_nVLOAD), digitalRead(PIN_nVISLE),
                      g_inputs.ntc_hot, g_inputs.vg2, g_inputs.vi2, g_inputs.vgi,
                      g_can_tx_enabled ? "EIN" : "AUS");
    }

    // 8. Serial-Befehle
    while (Serial.available()) {
        char c = (char)Serial.read();
        switch (c) {
            case 'a': g_mode = MODE_AUTO;        Serial.println("[Serial] Modus: Auto");       break;
            case 'g': g_mode = MODE_HAND_GRID;   Serial.println("[Serial] Modus: Hand:Netz");  break;
            case 'i': g_mode = MODE_HAND_ISLAND; Serial.println("[Serial] Modus: Hand:Insel"); break;
            case 'o': g_mode = MODE_HAND_OFF;    Serial.println("[Serial] Modus: Hand:Aus");   break;
            case 'c': sm_clear_error(); g_mode = MODE_AUTO;
                      Serial.println("[Serial] Fehler quittiert");                              break;
            case 't':
                g_can_tx_enabled = !g_can_tx_enabled;
                Serial.printf("[Serial] CAN TX: %s\n", g_can_tx_enabled ? "EIN" : "AUS");
                break;
            case 'd': {
                // Sofort-Debug-Dump aller I/O und ADC
                float rg, rb, rl;
                adc_read_raw_voltages(rg, rb, rl);
                Serial.println("=== DEBUG DUMP ===");
                Serial.printf("  Zeit:        %lu ms\n", millis());
                Serial.printf("  Zustand:     %s | Modus: %s\n", STATE_NAMES[state], MODE_NAMES[g_mode]);
                Serial.println("  -- ADC --");
                Serial.printf("  V_Grid:      %.1f V  (roh: %.4f V_ADC, Faktor: %.1f)\n",  s.v_grid, rg, ADC_GRID_SCALE);
                Serial.printf("  V_Batt:      %.1f V  (roh: %.4f V_ADC, Faktor: %.1f)\n",  s.v_batt, rb, ADC_BATT_DIVIDER);
                Serial.printf("  V_Load:      %.1f V  (roh: %.4f V_ADC, Faktor: %.1f)\n",  s.v_load, rl, ADC_GRID_SCALE);
                Serial.printf("  SOC:         %d %%\n", s.soc);
                Serial.println("  -- GPIO Optokoppler (INPUT_PULLUP, LOW=aktiv) --");
                Serial.printf("  PIN %2d nVGRID:    %s (%s)\n", PIN_nVGRID,    digitalRead(PIN_nVGRID)    ? "HIGH" : "LOW ", s.grid_ok ? "Netz OK"    : "kein Netz");
                Serial.printf("  PIN %2d nVGRID_OV: %s (%s)\n", PIN_nVGRID_OV, digitalRead(PIN_nVGRID_OV) ? "HIGH" : "LOW ", s.grid_ov ? "ÜBERSPANNUNG" : "OK");
                Serial.printf("  PIN %2d nVLOAD:    %s (%s)\n", PIN_nVLOAD,    digitalRead(PIN_nVLOAD)    ? "HIGH" : "LOW ", s.load_on ? "Last an"    : "keine Last");
                Serial.printf("  PIN %2d nVISLE:    %s (%s)\n", PIN_nVISLE,    digitalRead(PIN_nVISLE)    ? "HIGH" : "LOW ", s.isle_ok ? "Insel OK"   : "kein Insel");
                Serial.println("  -- PCA9555 (letzter ISR-Wert) --");
                Serial.printf("  Init:        %s\n", pca9555_is_ok() ? "OK" : "FEHLER (kein I2C)");
                Serial.printf("  NTC_hot:     %d  VG2: %d  VI2: %d  VGI: %d\n",
                              g_inputs.ntc_hot, g_inputs.vg2, g_inputs.vi2, g_inputs.vgi);
                Serial.printf("  IO[0..2]:    %d  %d  %d\n",
                              g_inputs.io[0], g_inputs.io[1], g_inputs.io[2]);
                Serial.println("  -- CAN --");
                Serial.printf("  MCP2515:     %s\n", g_can_init_ok ? "OK" : "FEHLER");
                Serial.printf("  TX:          %s\n", g_can_tx_enabled ? "EIN" : "AUS (Beobachter-Modus)");
                Serial.println("==================");
                break;
            }
        }
    }

    // 9. Loop-Timing: 100ms Zykluszeit einhalten
    uint32_t elapsed = millis() - cycle_start;
    if (elapsed < CYCLE_MS) {
        delay(CYCLE_MS - elapsed);
    }
}

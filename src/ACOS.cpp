#include "Arduino.h"
#include <M5Unified.h>
#include <mcp2515.h>
#include "config.h"
#include <Wire.h>
#include <SPI.h>

#include "pca9555_module.h"
#include "can_module.h"
#include "display_module.h"

// ================================================================
//  Anzeigedaten  (werden in loop() befüllt und an display_update()
//  übergeben — Platzhalter bis echte Messwerte vorliegen)
// ================================================================
static DisplayData g_disp = {
    .u_grid = 0.0f,
    .f_grid = 0.0f,
    .soc    = 0,
    .batt_v = 0.0f,
    .status = "Initialisierung..."
};

// ================================================================
//  PCA9555 — Interrupt-State
//  Der Callback läuft im IRQ-Task-Kontext → nur volatile State
//  setzen, Logik unten in handle_io_change() bzw. loop().
// ================================================================
// volatile nur für das Flag nötig (wird in loop() gepollt).
// g_inputs liegt im FreeRTOS-Task-Kontext — kein volatile erforderlich,
// FreeRTOS sorgt intern für Memory Barriers.
static volatile bool  g_io_updated = false;
static Pca9555Inputs  g_inputs     = {};

static void on_io_change(const Pca9555Inputs& in) {
    g_inputs     = in;     // einfache Struct-Zuweisung, kein volatile-Konflikt
    g_io_updated = true;
}

// ================================================================
//  Eingangsänderungen verarbeiten
//  Wird aus loop() aufgerufen, sobald g_io_updated gesetzt ist.
// ================================================================
static void handle_io_change() {
    // Lokale Kopie anlegen, Flag zurücksetzen
    Pca9555Inputs in = g_inputs;
    g_io_updated     = false;

    // --- NTC Übertemperatur (IO0_3) ---------------------------------
    if (in.ntc_hot) {
        Serial.println("[WARNUNG] NTC Übertemperatur erkannt!");
        // TODO: Relais abschalten, Fehler-State setzen
    }

    // --- Spannungsüberwachung (Port 1, OC active-LOW) ---------------
    if (in.vg2) {
        Serial.println("[IO] Spannung L_G2 erkannt (VG2)");
    } else {
        Serial.println("[IO] Spannung L_G2 verloren (VG2)");
    }

    if (in.vi2) {
        Serial.println("[IO] Spannung L_I2 erkannt (VI2)");
    } else {
        Serial.println("[IO] Spannung L_I2 verloren (VI2)");
    }

    if (in.vgi) {
        Serial.println("[IO] Spannung L_GI erkannt (VGI)");
    } else {
        Serial.println("[IO] Spannung L_GI verloren (VGI)");
    }

    // --- Externe I/Os (IO1_5–IO1_7) ---------------------------------
    for (uint8_t i = 0; i < 3; i++) {
        Serial.printf("[IO] IO_%u = %s\n", i, in.io[i] ? "HIGH" : "LOW");
    }
}

// ================================================================
//  Setup
// ================================================================
void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    Wire.begin();

    // --- PCA9555 initialisieren (INT-Pin aus config.h) ---------------
    if (!pca9555_init(PIN_PCA9555_INT, on_io_change)) {
        Serial.println("[FEHLER] IC9 PCA9555 nicht erreichbar!");
        // TODO: Fehler anzeigen / System stoppen
    } else {
        Serial.println("[OK] IC9 PCA9555 initialisiert.");
    }

    /* --- CAN initialisieren ------------------------------------------
    if (!can_init()) {
        Serial.println("[FEHLER] CAN MCP2515 nicht erreichbar!");
    } else {
        Serial.println("[OK] CAN initialisiert.");
    }
    */

    // ----------------------------------------------------------------
    //  Beispiele: Ausgangsfunktionen
    // ----------------------------------------------------------------

    // Netz-Relais einschalten
    pca9555_set_grid_on(true);
    delay(100);
    pca9555_set_grid_on(false);

    // Insel-Relais einschalten
    pca9555_set_isle_on(true);
    delay(100);
    pca9555_set_isle_on(false);

    // Grid/Island Umschaltung: Grid-Modus aktivieren
    pca9555_set_gi_sel(true);   // true = Grid

    // Analogkanal-Auswahl (0–3 → ASEL1/ASEL2)
    pca9555_set_asel(0);        // Kanal 0: ASEL1=0, ASEL2=0
    pca9555_set_asel(1);        // Kanal 1: ASEL1=1, ASEL2=0
    pca9555_set_asel(2);        // Kanal 2: ASEL1=0, ASEL2=1
    pca9555_set_asel(3);        // Kanal 3: ASEL1=1, ASEL2=1
    pca9555_set_asel(0);        // zurück auf Kanal 0

    // ----------------------------------------------------------------
    //  Beispiele: direkter Lesezugriff (einmalig, z.B. Startup-Check)
    //  Für laufende Überwachung → Interrupt-Callback nutzen (s.o.)
    // ----------------------------------------------------------------
    bool ntc  = pca9555_get_ntc_hot();
    bool vg2  = pca9555_get_vg2();
    bool vi2  = pca9555_get_vi2();
    bool vgi  = pca9555_get_vgi();
    bool io0  = pca9555_get_io(0);
    bool io1  = pca9555_get_io(1);
    bool io2  = pca9555_get_io(2);

    Serial.printf("[Startup] NTC_HOT=%d  VG2=%d  VI2=%d  VGI=%d  IO=[%d,%d,%d]\n",
                  ntc, vg2, vi2, vgi, io0, io1, io2);

    display_init();
    g_disp.status = "Bereit";
}


void loop() {
    M5.update();

    // --- PCA9555 Eingangsänderung verarbeiten -----------------------
    if (g_io_updated) {
        handle_io_change();
    }

    // --- Display Touch auswerten ------------------------------------
    DispEvent evt = display_handle_touch();
    switch (evt) {
        case DISP_EVT_TO_MANUAL:
            g_disp.status = "Manuell";
            Serial.println("[Display] → Manual-View");
            break;
        case DISP_EVT_TO_AUTO:
            g_disp.status = "Auto";
            Serial.println("[Display] → Auto-View");
            break;
        case DISP_EVT_MODE_ISLAND:
            g_disp.status = "Modus: Island";
            Serial.println("[Display] Modus: Island");
            pca9555_set_isle_on(true);
            pca9555_set_grid_on(false);
            pca9555_set_gi_sel(false);
            break;
        case DISP_EVT_MODE_OFF:
            g_disp.status = "Modus: OFF";
            Serial.println("[Display] Modus: OFF");
            pca9555_set_isle_on(false);
            pca9555_set_grid_on(false);
            break;
        case DISP_EVT_MODE_GRID:
            g_disp.status = "Modus: Grid";
            Serial.println("[Display] Modus: Grid");
            pca9555_set_grid_on(true);
            pca9555_set_isle_on(false);
            pca9555_set_gi_sel(true);
            break;
        default:
            break;
    }

    // --- Display aktualisieren (intern auf 100 ms gedrosselt) -------
    display_update(g_disp);
}

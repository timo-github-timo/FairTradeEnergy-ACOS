#pragma once
// ================================================================
//  adc_reader.h  —  ADC-Messung + SOC-Berechnung
//
//  Aufbau:
//    ESP32-S3 ADC (GPIO10) ← IC14 Analog-Multiplexer
//    Kanalwahl: pca9555_set_asel() → IO0_4/IO0_5
//
//  Kanäle:
//    0 = Netzspannung  (TR2 → IC11/IC12 OPV → IC14)   Skalierung: ~150:1
//    1 = Batterie      (R59/R60 Teiler)                Skalierung: 101:1
//    2 = Lastspannung  (gleiche Messkette wie Netz)    Skalierung: ~150:1
//
//  SOC:
//    LiFePO4 16S Lookup-Tabelle + linearer Interpolation
//    EMA-Filter (α=0.1) gegen Messrauschen
//
//  Abhängigkeiten: config.h, pca9555_module.h
// ================================================================

#include "config.h"

// Einmalig in setup() aufrufen (nach pca9555_init).
void adc_init();

// Alle drei Kanäle nacheinander lesen.
// Schaltet Multiplexer via pca9555_set_asel() — nicht ISR-sicher.
// Gibt kalibierte Spannungen in Volt zurück.
void adc_read_all(float& v_grid_out, float& v_batt_out, float& v_load_out);

// SOC aus Batteriespannung berechnen (LFP 16S LUT + EMA-Filter).
// Erster Aufruf initialisiert den EMA-Filter direkt (kein Einschwingen).
int8_t adc_calc_soc(float v_batt);

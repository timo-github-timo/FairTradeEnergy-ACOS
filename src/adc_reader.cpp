// ================================================================
//  adc_reader.cpp  —  ADC-Messung + SOC-Berechnung
// ================================================================

#include "adc_reader.h"
#include "pca9555_module.h"
#include <Arduino.h>

// ----------------------------------------------------------------
//  SOC-Lookup-Tabelle  (LiFePO4, 16S)
//  Spannungen aufsteigend, lineare Interpolation zwischen Stützpunkten.
//  Anpassen falls andere Batterie-Chemie / Zellanzahl verwendet wird.
// ----------------------------------------------------------------
static const float  LUT_V[]   = { 40.0f, 44.0f, 46.4f, 48.0f, 49.6f,
                                   50.4f, 51.2f, 51.6f, 52.0f, 52.8f,
                                   53.6f, 54.4f, 55.2f };
static const int8_t LUT_SOC[] = {     0,     5,    10,    20,    30,
                                      40,    50,    60,    70,    80,
                                      90,    95,   100 };
static const int    LUT_N     = sizeof(LUT_V) / sizeof(LUT_V[0]);

static constexpr float EMA_ALPHA = 0.1f;
static float           soc_ema   = -1.0f;  // <0 = nicht initialisiert

// ----------------------------------------------------------------
//  Init
// ----------------------------------------------------------------
void adc_init() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);    // 0–3.3V Eingangsbereich
    pca9555_set_asel(ADC_CH_GRID);     // Startkanal vorwählen
}

// ----------------------------------------------------------------
//  Einen Kanal lesen (Oversampling + Settling-Time)
// ----------------------------------------------------------------
static float read_channel(uint8_t channel) {
    pca9555_set_asel(channel);
    delayMicroseconds(ADC_SETTLING_US);

    uint32_t sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLING; i++) {
        sum += analogRead(PIN_ADC);
    }
    return ((float)sum / ADC_OVERSAMPLING) * (ADC_VREF / ADC_RESOLUTION);
}

// Letzter Rohwert (V_ADC vor Skalierung) — für Debug-Ausgaben
static float _raw_grid = 0.0f;
static float _raw_batt = 0.0f;
static float _raw_load = 0.0f;

// ----------------------------------------------------------------
void adc_read_all(float& v_grid_out, float& v_batt_out, float& v_load_out) {
    _raw_grid = read_channel(ADC_CH_GRID);
    _raw_batt = read_channel(ADC_CH_BATTERY);
    _raw_load = read_channel(ADC_CH_LOAD);

    v_grid_out = _raw_grid * ADC_GRID_SCALE;
    v_batt_out = _raw_batt * ADC_BATT_DIVIDER;
    v_load_out = _raw_load * ADC_GRID_SCALE;  // gleiche Messkette wie Netz
}

// ----------------------------------------------------------------
void adc_read_raw_voltages(float& raw_grid, float& raw_batt, float& raw_load) {
    raw_grid = _raw_grid;
    raw_batt = _raw_batt;
    raw_load = _raw_load;
}

// ----------------------------------------------------------------
int8_t adc_calc_soc(float v_batt) {
    float soc_raw;

    if (v_batt <= LUT_V[0]) {
        soc_raw = LUT_SOC[0];
    } else if (v_batt >= LUT_V[LUT_N - 1]) {
        soc_raw = LUT_SOC[LUT_N - 1];
    } else {
        soc_raw = LUT_SOC[0];
        for (int i = 0; i < LUT_N - 1; i++) {
            if (v_batt <= LUT_V[i + 1]) {
                float t = (v_batt - LUT_V[i]) / (LUT_V[i + 1] - LUT_V[i]);
                soc_raw = LUT_SOC[i] + t * (LUT_SOC[i + 1] - LUT_SOC[i]);
                break;
            }
        }
    }

    // EMA-Filter: erster Aufruf initialisiert direkt
    if (soc_ema < 0.0f) {
        soc_ema = soc_raw;
    } else {
        soc_ema = EMA_ALPHA * soc_raw + (1.0f - EMA_ALPHA) * soc_ema;
    }

    return (int8_t)(soc_ema + 0.5f);  // runden auf nächste ganze Zahl
}

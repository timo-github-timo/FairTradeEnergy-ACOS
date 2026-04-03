# ACOS – Automatic Change-Over Switch · Projektdokumentation

> **Zweck:** Automatische Umschaltung von Verbrauchern zwischen Inselnetz (Batterie) und öffentlichem Netz (Grid) mit Schutzfunktionen und Touch-UI.

---

## 1. Projektstruktur

```
FairTradeEnergy-ACOS/
├── platformio.ini          # Build-Konfiguration (Board, Libs, Flags)
├── include/
│   └── config.h            # ALLE Pins, Schwellwerte, Konstanten
└── src/
    ├── ACOS.cpp            # setup() + loop() – Hauptzyklus 100 ms
    ├── state_machine.h/cpp # Kern-Logik: Zustandsautomat
    ├── adc_reader.h/cpp    # Spannungsmessung + SOC-Berechnung
    ├── can_module.h/cpp    # CAN-Bus TX/RX (MCP2515)
    ├── display_module.h/cpp# 320×240 Touchscreen-UI
    └── pca9555_module.h/cpp# I²C I/O-Expander (Relais, Mux)
```

**Zielhardware:** M5CoreS3SE (ESP32-S3) auf kundenspezifischer PCB mit Relais, Spannungssensoren, CAN-Interface und PCA9555 I²C-Expander.

---

## 2. Betriebsmodi & Zustände

### Modi (wählbar per Touch oder CAN)

| Modus | Beschreibung |
|---|---|
| `MODE_AUTO` | Automatik – schaltet basierend auf SOC und Netzstatus |
| `MODE_HAND_GRID` | Manuell: Zwingend Netzbetrieb |
| `MODE_HAND_ISLAND` | Manuell: Zwingend Inselbetrieb |
| `MODE_HAND_OFF` | Manuell: Verbraucher abgetrennt |

### Zustände (Zustandsautomat)

| Zustand | Bedeutung |
|---|---|
| `STATE_OFF` | Verbraucher getrennt |
| `STATE_ISLAND` | Batteriebetrieb aktiv |
| `STATE_GRID` | Netzbetrieb aktiv |
| `STATE_TRANSITION_TO_GRID` | Umschaltung → Netz (Break-before-Make) |
| `STATE_TRANSITION_TO_ISLAND` | Umschaltung → Insel (Break-before-Make) |
| `STATE_ERROR` | Sicherheitsabschaltung (Überspannung/Übertemperatur) |

---

## 3. Kern-Snippets

### 3.1 Hauptzyklus (`ACOS.cpp`)

```cpp
const uint32_t CYCLE_MS  = 100;   // Hauptzyklus: 100 ms
const uint32_t CAN_TX_MS = 1000;  // CAN-Status: alle 1 s

void loop() {
    adc_reader_update();       // Spannung + SOC lesen
    state_machine_update();    // Zustände + Schalten
    display_module_update();   // UI aktualisieren
    // CAN TX alle 1 s
}
```

### 3.2 Globale Sicherheitsprüfung (`state_machine.cpp`)

```cpp
// Überschreibt ALLE anderen Logiken:
if (s.ntc_hot)  { set_error("Uebertemperatur");  return; }
if (s.grid_ov)  { set_error("Netz-Ueberspannung"); return; }
```

### 3.3 Automatik-Logik – Beispiel aus STATE_OFF

```cpp
case STATE_OFF:
    if (mode == MODE_AUTO) {
        if (s.soc >= THR_SOC_MIN && !s.grid_ok && s.v_load < THR_SWITCH_V)
            enter_state(STATE_TRANSITION_TO_ISLAND); // Batterie ok, kein Netz
        else if (s.soc < THR_SOC_MAX && s.grid_ok && s.v_load < THR_SWITCH_V)
            enter_state(STATE_TRANSITION_TO_GRID);   // Netz ok, Batterie nicht voll
    }
    break;
```

### 3.4 Spannungsmessung mit Oversampling (`adc_reader.cpp`)

```cpp
static float read_channel(uint8_t channel) {
    pca9555_set_asel(channel);           // Mux umschalten
    delayMicroseconds(ADC_SETTLING_US); // 100 µs Einschwingen
    uint32_t sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLING; i++)  // 16 Samples
        sum += analogRead(PIN_ADC);
    return ((float)sum / ADC_OVERSAMPLING) * (ADC_VREF / ADC_RESOLUTION);
}
```

SOC-Berechnung: Lookup-Table (LiFePO4 16S) + lineare Interpolation + EMA-Filter (α = 0,1).

### 3.5 I²C Interrupt-sicheres I/O (`pca9555_module.cpp`)

```cpp
// ISR – minimaler Kontext, nur Signal:
static void IRAM_ATTR pca9555_isr() {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(_irq_sem, &woken);
    portYIELD_FROM_ISR(woken);
}
// Separater FreeRTOS-Task erledigt die eigentliche I²C-Kommunikation
```

### 3.6 CAN-Protokoll (`can_module.cpp`)

**Status TX (ID `0x101`, 1 Hz):**

| Byte | Inhalt |
|---|---|
| 0 | State (0=OFF … 5=ERROR) |
| 1 | Mode (0=AUTO … 3=HAND_OFF) |
| 2 | SOC [%] |
| 3–4 | Netzspannung (uint16 BE, ×10) |
| 5–6 | Batteriespannung (uint16 BE, ×10) |
| 7 | Flags: bit0=grid_ok, bit1=grid_ov, bit2=load_on, bit3=ntc_hot |

**Kommando RX (ID `0x201`):**

| Byte 0 | Befehl |
|---|---|
| `0x00` | AUTO |
| `0x01` | Manuell Netz |
| `0x02` | Manuell Insel |
| `0x03` | Manuell Aus |
| `0x04` | Fehler quittieren |

---

## 4. Konfigurationsparameter für Live-Betrieb

> Alle Parameter in `include/config.h` und `src/can_module.cpp`.

### 4.1 Pins – Muss gegen PCB-Schaltplan verifiziert werden

| Define | Wert | Status |
|---|---|---|
| `PIN_I2C_SDA` | 12 | OK |
| `PIN_I2C_SCL` | 11 | OK |
| `PIN_PCA9555_INT` | 1 | OK |
| `PIN_ADC` | 10 | OK |
| `PIN_nVGRID` | 7 | OK |
| `PIN_nVGRID_OV` | 5 | OK |
| `PIN_nVLOAD` | 8 | OK |
| `PIN_nVISLE` | 21 | OK |
| `PIN_CAN_CS` | 38 | **TODO – verifizieren** |
| `PIN_CAN_INT` | 45 | **TODO – verifizieren** |
| `PIN_RS485_WR` | 46 | **TODO – verifizieren** |

### 4.2 ADC-Skalierung – Kalibrierung erforderlich

| Define | Wert | Hinweis |
|---|---|---|
| `ADC_BATT_DIVIDER` | 101.0 | R59(10MΩ) + R60(100kΩ) – prüfen |
| `ADC_GRID_SCALE` | 150.0 | **PLATZHALTER – kalibrieren!** |
| `ADC_OVERSAMPLING` | 16 | Samples pro Messung |
| `ADC_SETTLING_US` | 100 | Einschwingzeit nach Mux [µs] |

### 4.3 SOC-Schwellwerte

| Define | Wert | Bedeutung |
|---|---|---|
| `THR_SOC_EMERGENCY` | 5 % | Notabschaltung |
| `THR_SOC_MIN` | 15 % | Untergrenze → Grid bevorzugen |
| `THR_SOC_MAX` | 95 % | Obergrenze → Insel bevorzugen |
| `THR_SWITCH_V` | 50,0 V | Verbraucher gilt als „aus" |
| `TIMEOUT_RELAY_MS` | 3000 | Max. Wartezeit Relaisumschaltung [ms] |

### 4.4 Batterie-Lookup-Table (LiFePO4 16S)

```c
static const float  LUT_V[]   = { 40.0, 44.0, 46.4, 48.0, 49.6,
                                   50.4, 51.2, 51.6, 52.0, 52.8,
                                   53.6, 54.4, 55.2 };
static const int8_t LUT_SOC[] = {    0,    5,   10,   20,   30,
                                     40,   50,   60,   70,   80,
                                     90,   95,  100 };
```

Bei anderer Batteriechemie oder Zellenanzahl muss diese Tabelle angepasst werden.

### 4.5 CAN-Bus

| Parameter | Wert | Anpassen wenn… |
|---|---|---|
| CAN Bitrate | 500 kbps | Andere Geräte am Bus |
| Oszillator | `MCP_8MHZ` | Anderer Quarz verbaut → `MCP_16MHZ` |
| TX-ID | `0x101` | Konflikt mit anderen Knoten |
| RX-ID | `0x201` | Konflikt mit anderen Knoten |

### 4.6 I²C-Adresse PCA9555

```c
#define PCA9555_I2C_ADDR  0x27   // A0=A1=A2=HIGH
// Ändern wenn Address-Pins anders bestückt
```

### 4.7 Zykluszeiten

| Define | Wert | Beschreibung |
|---|---|---|
| `CYCLE_MS` | 100 ms | Hauptzyklus |
| `SERIAL_MS` | 2000 ms | Serieller Debug-Output |
| `CAN_TX_MS` | 1000 ms | CAN Status-Telegramm |

---

## 5. Debug-Funktionen (ohne Platine)

### 5.1 Serial-Befehle (115200 Baud)

| Taste | Funktion |
|---|---|
| `a` | Modus: Auto |
| `g` | Modus: Hand Netz |
| `i` | Modus: Hand Insel |
| `o` | Modus: Hand Aus |
| `c` | Fehler quittieren |
| `t` | CAN TX umschalten (EIN/AUS) |
| `d` | Sofort-Debug-Dump (alle I/O + ADC) |

### 5.2 CAN Beobachter-Modus

CAN TX ist **standardmäßig deaktiviert** (`g_can_tx_enabled = false` in `ACOS.cpp`).  
CAN RX läuft immer, sofern MCP2515 erreichbar.  
Mit `t` kann TX zur Laufzeit ein- und ausgeschaltet werden.

> Zum dauerhaften Aktivieren: `g_can_tx_enabled = true` als Initialwert setzen.

### 5.3 Automatischer 2s-Debug-Output (Serial)

```
[ACOS] t=12345ms | AUS | Auto | SOC=0%
  | VGrid=0.0V(roh=0.001V) | VBatt=0.0V(roh=0.002V) | VLoad=0.0V(roh=0.001V)
  | GPIO: nVGRID=1 nVGRID_OV=1 nVLOAD=1 nVISLE=1
  | PCA: NTC=0 VG2=0 VI2=0 VGI=0 | CAN-TX=AUS
```

GPIO-Werte: `1` = HIGH (Optokoppler offen/inaktiv), `0` = LOW (aktiv).  
Rohwert ADC = V_ADC vor Hardware-Skalierung → nützlich zur Kalibrierung von `ADC_GRID_SCALE` und `ADC_BATT_DIVIDER`.

### 5.4 Sofort-Dump mit `d`

```
=== DEBUG DUMP ===
  Zeit:        12345 ms
  Zustand:     AUS | Modus: Auto
  -- ADC --
  V_Grid:      0.0 V  (roh: 0.0012 V_ADC, Faktor: 150.0)
  V_Batt:      0.0 V  (roh: 0.0008 V_ADC, Faktor: 101.0)
  V_Load:      0.0 V  (roh: 0.0010 V_ADC, Faktor: 150.0)
  SOC:         0 %
  -- GPIO Optokoppler (INPUT_PULLUP, LOW=aktiv) --
  PIN  7 nVGRID:    HIGH (kein Netz)
  PIN  5 nVGRID_OV: HIGH (OK)
  PIN  8 nVLOAD:    HIGH (keine Last)
  PIN 21 nVISLE:    HIGH (kein Insel)
  -- PCA9555 (letzter ISR-Wert) --
  Init:        FEHLER (kein I2C)
  NTC_hot: 0  VG2: 0  VI2: 0  VGI: 0
  IO[0..2]: 0  0  0
  -- CAN --
  MCP2515:     FEHLER
  TX:          AUS (Beobachter-Modus)
==================
```

### 5.5 Touch-Debug

Jeder Touch-Event wird ins Serial geloggt:
```
[Touch] x=245 y=120  View=AUTO
```
Dient zur Überprüfung ob Touch-Events ankommen und ob Koordinaten stimmen.

### 5.6 Verhalten ohne Platine (kein PCB)

| Modul | Verhalten |
|---|---|
| PCA9555 | Init schlägt fehl → alle I²C-Operationen werden übersprungen (kein Timeout-Block mehr) |
| ADC | Liest PIN 10 direkt – Werte sind bedeutungslos ohne Mux |
| CAN MCP2515 | Init schlägt fehl → TX und RX deaktiviert |
| Display | Läuft normal, Force-Refresh alle 5 s |
| Touch | Funktioniert normal (unabhängig von Platine) |

---

## 7. Noch nicht implementiert (v1.0)

- Netzfrequenzmessung (aktuell hardcoded 50 Hz)
- RS485-Kommunikation (Hardware vorhanden, Software fehlt)
- Over-the-Air Updates
- S0-Impuls-Zählung (Energiemessung)
- Lastabwurf über externen Kontakt

---

## 8. Deployment-Checkliste

- [ ] Alle Pins gegen Schaltplan/PCB verifiziert (bes. CAN, RS485)
- [ ] `ADC_GRID_SCALE` kalibriert (Messung gegen bekannte Spannung)
- [ ] Batteriechemie und Zellanzahl stimmt mit LUT überein
- [ ] CAN-Oszillatorfrequenz korrekt (`MCP_8MHZ` vs. `MCP_16MHZ`)
- [ ] SOC-Schwellwerte ans System angepasst
- [ ] CAN-IDs auf Konflikte im Netzwerk geprüft
- [ ] `PCA9555_I2C_ADDR` gegen Bestückungsplan geprüft
- [ ] Break-before-Make Relaissequenz im Lasttest bestätigt
- [ ] Schutzfunktionen (Übertemperatur, Überspannung) getestet
- [ ] Touch-UI auf echter Hardware getestet
- [ ] `APP_VERSION` in `platformio.ini` gesetzt (aktuell `"dev"`)

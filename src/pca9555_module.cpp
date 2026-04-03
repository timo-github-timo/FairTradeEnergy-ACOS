// ================================================================
//  pca9555_module.cpp  —  IC9 PCA9555PW I/O-Expander
//  Zielplattform: ESP32-S3 (M5CoreS3SE)
// ================================================================

#include "pca9555_module.h"
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static PCA9555           ic9(PCA9555_I2C_ADDR);
static Pca9555Callback   _callback    = nullptr;
static SemaphoreHandle_t _irq_sem     = nullptr;
static SemaphoreHandle_t _wire_mutex  = nullptr;  // schützt alle Wire-Zugriffe
static bool              _init_ok     = false;    // true nur wenn IC9 erreichbar

// ----------------------------------------------------------------
//  ISR  (IRAM_ATTR: liegt im internen RAM, läuft auch während
//        Flash-Zugriffen)
//  Gibt nur das Semaphor — kein I²C, kein malloc, keine Logik.
// ----------------------------------------------------------------
static void IRAM_ATTR pca9555_isr() {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(_irq_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

// ----------------------------------------------------------------
//  IRQ-Task  (Core 1, blockiert auf Semaphor — kein Busy-Wait)
//
//  Wire-Zugriff mit Mutex geschützt, damit kein Bus-Konflikt mit
//  den Ausgabe-Funktionen (pca9555_set_*) entsteht die ebenfalls
//  Wire benutzen.
//
//  CALLBACK-HINWEIS: Im Callback nur volatile-Zustände setzen oder
//  ein Flag für loop()/eigenen Task. Keine langen Operationen.
// ----------------------------------------------------------------
static void pca9555_irq_task(void* /*arg*/) {
    for (;;) {
        xSemaphoreTake(_irq_sem, portMAX_DELAY);

        // Wire-Mutex holen bevor I²C angesprochen wird
        xSemaphoreTake(_wire_mutex, portMAX_DELAY);

        // Beide Ports sequenziell in einer Transaktion lesen
        Wire.beginTransmission(PCA9555_I2C_ADDR);
        Wire.write(NXP_INPUT);
        Wire.endTransmission(false);    // Repeated-Start
        int received = Wire.requestFrom((uint8_t)PCA9555_I2C_ADDR, (uint8_t)2);

        uint8_t port0 = Wire.read();
        uint8_t port1 = Wire.read();

        xSemaphoreGive(_wire_mutex);

        // Bus-Fehler: kein Callback mit ungültigen Daten
        if (received != 2) {
            continue;
        }

        Pca9555Inputs in;
        in.ntc_hot = (port0 >> 3) & 0x01;      // IO0_3, active-HIGH
        in.vg2     = !((port1 >> 0) & 0x01);   // IO1_0, active-LOW → invertiert
        in.vi2     = !((port1 >> 1) & 0x01);   // IO1_1, active-LOW → invertiert
        in.vgi     = !((port1 >> 2) & 0x01);   // IO1_2, active-LOW → invertiert
        in.io[0]   =  (port1 >> 5) & 0x01;     // IO1_5
        in.io[1]   =  (port1 >> 6) & 0x01;     // IO1_6
        in.io[2]   =  (port1 >> 7) & 0x01;     // IO1_7

        if (_callback) {
            _callback(in);
        }
    }
}

// ----------------------------------------------------------------
//  Init
// ----------------------------------------------------------------

bool pca9555_init(uint8_t irqPin, Pca9555Callback callback) {
    _callback = callback;

    // Mutex und Semaphor anlegen — vor Task-Start und Interrupt
    _wire_mutex = xSemaphoreCreateMutex();
    if (_wire_mutex == nullptr) return false;

    _irq_sem = xSemaphoreCreateBinary();
    if (_irq_sem == nullptr) return false;

    // IC9 erreichbar? (Wire-Zugriff hier noch ohne Mutex —
    // Task und Interrupt existieren noch nicht)
    if (!ic9.begin()) {
        return false;
    }

    // Port 0 — Ausgänge
    ic9.pinMode(PIN_GRID_ON, OUTPUT);
    ic9.pinMode(PIN_ISLE_ON, OUTPUT);
    ic9.pinMode(PIN_GI_SEL,  OUTPUT);
    ic9.pinMode(PIN_ASEL1,   OUTPUT);
    ic9.pinMode(PIN_ASEL2,   OUTPUT);

    // Port 0 — Eingang
    ic9.pinMode(PIN_NTC_HOT, INPUT);

    // Port 1 — Eingänge (OC, active-LOW)
    ic9.pinMode(PIN_nVG2, INPUT);
    ic9.pinMode(PIN_nVI2, INPUT);
    ic9.pinMode(PIN_nVGI, INPUT);

    // Port 1 — Externe I/Os (als Eingänge)
    ic9.pinMode(PIN_IO_0, INPUT);
    ic9.pinMode(PIN_IO_1, INPUT);
    ic9.pinMode(PIN_IO_2, INPUT);

    // Ausgänge auf definierten Anfangszustand
    ic9.digitalWrite(PIN_GRID_ON, LOW);
    ic9.digitalWrite(PIN_ISLE_ON, LOW);
    ic9.digitalWrite(PIN_GI_SEL,  LOW);
    ic9.digitalWrite(PIN_ASEL1,   LOW);
    ic9.digitalWrite(PIN_ASEL2,   LOW);

    // IRQ-Task starten — Core 1, Priorität 5
    xTaskCreatePinnedToCore(
        pca9555_irq_task,
        "pca9555_irq",
        2048,
        nullptr,
        5,
        nullptr,
        1
    );

    // Hardware-Interrupt registrieren (nach Task-Start)
    ::pinMode(irqPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(irqPin), pca9555_isr, FALLING);

    _init_ok = true;
    return true;
}

// ----------------------------------------------------------------
//  Ausgänge — Wire-Mutex schützt gegen parallelen IRQ-Task-Zugriff
// ----------------------------------------------------------------

void pca9555_set_grid_on(bool on) {
    if (!_init_ok) return;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    ic9.digitalWrite(PIN_GRID_ON, on ? HIGH : LOW);
    xSemaphoreGive(_wire_mutex);
}

void pca9555_set_isle_on(bool on) {
    if (!_init_ok) return;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    ic9.digitalWrite(PIN_ISLE_ON, on ? HIGH : LOW);
    xSemaphoreGive(_wire_mutex);
}

void pca9555_set_gi_sel(bool grid) {
    if (!_init_ok) return;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    ic9.digitalWrite(PIN_GI_SEL, grid ? HIGH : LOW);
    xSemaphoreGive(_wire_mutex);
}

void pca9555_set_asel(uint8_t channel) {
    if (!_init_ok) return;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    ic9.digitalWrite(PIN_ASEL1, (channel & 0x01) ? HIGH : LOW);
    ic9.digitalWrite(PIN_ASEL2, (channel & 0x02) ? HIGH : LOW);
    xSemaphoreGive(_wire_mutex);
}

// ----------------------------------------------------------------
//  Direkter Lesezugriff via I²C (außerhalb des Interrupt-Pfads)
// ----------------------------------------------------------------

bool pca9555_is_ok() { return _init_ok; }

bool pca9555_get_ntc_hot() {
    if (!_init_ok) return false;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    bool val = ic9.digitalRead(PIN_NTC_HOT) == HIGH;
    xSemaphoreGive(_wire_mutex);
    return val;
}

bool pca9555_get_vg2() {
    if (!_init_ok) return false;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    bool val = ic9.digitalRead(PIN_nVG2) == LOW;
    xSemaphoreGive(_wire_mutex);
    return val;
}

bool pca9555_get_vi2() {
    if (!_init_ok) return false;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    bool val = ic9.digitalRead(PIN_nVI2) == LOW;
    xSemaphoreGive(_wire_mutex);
    return val;
}

bool pca9555_get_vgi() {
    if (!_init_ok) return false;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    bool val = ic9.digitalRead(PIN_nVGI) == LOW;
    xSemaphoreGive(_wire_mutex);
    return val;
}

bool pca9555_get_io(uint8_t n) {
    if (!_init_ok) return false;
    xSemaphoreTake(_wire_mutex, portMAX_DELAY);
    bool val = false;
    switch (n) {
        case 0:  val = ic9.digitalRead(PIN_IO_0) == HIGH; break;
        case 1:  val = ic9.digitalRead(PIN_IO_1) == HIGH; break;
        case 2:  val = ic9.digitalRead(PIN_IO_2) == HIGH; break;
    }
    xSemaphoreGive(_wire_mutex);
    return val;
}

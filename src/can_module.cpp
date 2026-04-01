// ================================================================
//  can_module.cpp  —  CAN-Bus Kommunikation (MCP2515)
// ================================================================

#include "can_module.h"
#include <SPI.h>

// ----------------------------------------------------------------
//  CAN-IDs
// ----------------------------------------------------------------
static const uint32_t CAN_ID_STATUS  = 0x101;   // TX: ACOS → Bus
static const uint32_t CAN_ID_COMMAND = 0x201;   // RX: Bus  → ACOS

// RX-Befehlswerte (Byte 0 im Command-Frame)
// 0x00 = AUTO → default-Branch → MODE_AUTO (kein Symbol nötig)
static const uint8_t CMD_HAND_GRID   = 0x01;
static const uint8_t CMD_HAND_ISLAND = 0x02;
static const uint8_t CMD_HAND_OFF    = 0x03;
static const uint8_t CMD_CLEAR_ERROR = 0x04;

static MCP2515 mcp2515(PIN_CAN_CS);
static bool    _initialized = false;

// ----------------------------------------------------------------
bool can_init() {
    // SPI-Bus mit den korrekten Pins konfigurieren (vor MCP2515-Zugriff)
    SPI.begin(PIN_CAN_SCK, PIN_CAN_MISO, PIN_CAN_MOSI, PIN_CAN_CS);

    mcp2515.reset();

    // Bitrate: 500 kbps, Quarzfrequenz: 8 MHz
    // HINWEIS: MCP_8MHZ → MCP_16MHZ ändern falls 16MHz-Quarz verbaut ist
    if (mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) != MCP2515::ERROR_OK) {
        return false;
    }
    if (mcp2515.setNormalMode() != MCP2515::ERROR_OK) {
        return false;
    }

    _initialized = true;
    return true;
}

// ----------------------------------------------------------------
void can_send_status(AcosState    state,
                     OperatingMode mode,
                     int8_t       soc,
                     float        v_grid,
                     float        v_batt,
                     uint8_t      flags) {
    if (!_initialized) return;

    uint16_t grid_x10 = (uint16_t)(v_grid * 10.0f);
    uint16_t batt_x10 = (uint16_t)(v_batt * 10.0f);

    struct can_frame frame;
    frame.can_id  = CAN_ID_STATUS;
    frame.can_dlc = 8;
    frame.data[0] = (uint8_t)state;
    frame.data[1] = (uint8_t)mode;
    frame.data[2] = (uint8_t)soc;
    frame.data[3] = (uint8_t)(grid_x10 >> 8);
    frame.data[4] = (uint8_t)(grid_x10 & 0xFF);
    frame.data[5] = (uint8_t)(batt_x10 >> 8);
    frame.data[6] = (uint8_t)(batt_x10 & 0xFF);
    frame.data[7] = flags;

    mcp2515.sendMessage(&frame);
}

// ----------------------------------------------------------------
OperatingMode can_receive(bool& clear_error_out) {
    clear_error_out = false;
    if (!_initialized) return MODE_AUTO;

    struct can_frame frame;
    if (mcp2515.readMessage(&frame) != MCP2515::ERROR_OK) {
        return MODE_AUTO;   // kein Frame verfügbar oder Fehler
    }

    if (frame.can_id != CAN_ID_COMMAND || frame.can_dlc < 1) {
        return MODE_AUTO;   // fremdes Frame ignorieren
    }

    switch (frame.data[0]) {
        case CMD_HAND_GRID:    return MODE_HAND_GRID;
        case CMD_HAND_ISLAND:  return MODE_HAND_ISLAND;
        case CMD_HAND_OFF:     return MODE_HAND_OFF;
        case CMD_CLEAR_ERROR:  clear_error_out = true; return MODE_AUTO;
        default:               return MODE_AUTO;
    }
}

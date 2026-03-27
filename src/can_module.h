#pragma once
// ================================================================
//  can_module.h  —  CAN-Bus Kommunikation (MCP2515)
//  Bibliothek : autowp/arduino-mcp2515  (Arduino Library Manager)
//
//  Einfaches Polling-Modell:
//    - can_send_status()   → im Loop periodisch aufrufen
//    - can_receive()       → im Loop polling, gibt Befehl zurück
//
//
//  Abhängigkeiten: config.h, <mcp2515.h>
// ================================================================

#include "config.h"
#include <mcp2515.h>    // autowp/arduino-mcp2515 — Arduino Library Manager



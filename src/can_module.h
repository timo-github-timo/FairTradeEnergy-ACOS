#pragma once
// ================================================================
//  can_module.h  —  CAN-Bus Kommunikation (MCP2515)
//  Bibliothek : autowp/arduino-mcp2515 @ v1.1.0
//
//  SPI-Pins : PIN_CAN_SCK / PIN_CAN_MISO / PIN_CAN_MOSI / PIN_CAN_CS
//             (alle in config.h definiert)
//
//  TX (0x101) — ACOS-Status, 1Hz
//    Byte 0 : AcosState     (STATE_OFF=0 .. STATE_ERROR=5)
//    Byte 1 : OperatingMode (MODE_AUTO=0 .. MODE_HAND_OFF=3)
//    Byte 2 : SOC           [%]
//    Byte 3 : GridV High    (uint16 = V * 10, Big-Endian)
//    Byte 4 : GridV Low
//    Byte 5 : BattV High    (uint16 = V * 10, Big-Endian)
//    Byte 6 : BattV Low
//    Byte 7 : Flags         bit0=grid_ok, bit1=grid_ov, bit2=load_on, bit3=ntc_hot
//
//  RX (0x201) — Fernbefehl
//    Byte 0 : 0x00=Auto  0x01=Hand:Netz  0x02=Hand:Insel
//             0x03=Hand:Aus  0x04=Fehler quittieren
//
//  Abhängigkeiten: config.h, <mcp2515.h>
// ================================================================

#include "config.h"
#include <mcp2515.h>

// Einmalig in setup() aufrufen.
// Gibt true zurück wenn MCP2515 erreichbar und konfiguriert ist.
// HINWEIS: SPI.begin() wird intern mit PIN_CAN_* aufgerufen.
//          MCP_8MHZ anpassen falls abweichende Quarzfrequenz verbaut.
bool can_init();

// Status-Frame senden — im loop() ca. 1x pro Sekunde aufrufen.
void can_send_status(AcosState    state,
                     OperatingMode mode,
                     int8_t       soc,
                     float        v_grid,
                     float        v_batt,
                     uint8_t      flags);

// Empfangene Fernbefehle abfragen (Polling).
// Rückgabe : neuer OperatingMode falls Befehl empfangen, sonst aktueller Modus
// clear_error_out : wird true gesetzt wenn Quittierungs-Befehl (0x04) empfangen
OperatingMode can_receive(bool& clear_error_out);

// Can bus MCP2515 header file for config
//#include <mcp2515.h>

#ifndef CAN_BUS_H
#define CAN_BUS_H

#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

// ==============================
// Configuration
// ==============================

#define CAN_CS_PIN   10
#define CAN_INT_PIN  9

#define CAN_SPEED CAN_500KBPS
#define CAN_CLOCK MCP_8MHZ

// ==============================
// CAN Frame Structure
// ==============================

struct CANFrame
{
    uint32_t id;
    uint8_t length;
    uint8_t data[8];
};

// ==============================
// CANBus Class
// ==============================

class CANBus
{
public:

    bool begin();
    bool available();
    bool read(CANFrame &frame);
    bool send(uint32_t id, uint8_t len, const uint8_t *data);

    void setNormalMode();
    void setListenMode();

    uint32_t getRxCount();
    uint32_t getTxCount();
    uint32_t getErrorCount();

private:

    MCP_CAN mcp = MCP_CAN(CAN_CS_PIN);

    uint32_t rxCount = 0;
    uint32_t txCount = 0;
    uint32_t errorCount = 0;
};

#endif
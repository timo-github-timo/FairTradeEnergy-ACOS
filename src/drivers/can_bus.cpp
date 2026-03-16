// MCP2515 CAN-Bus Logik und Implementierung
//#include <mcp2515.h>
#include <mcp_can.h>
#include "can_bus.h"

// ==============================
// Initialization
// ==============================

bool CANBus::begin()
{
    pinMode(CAN_INT_PIN, INPUT);

    SPI.begin();

    if (mcp.begin(MCP_ANY, CAN_SPEED, CAN_CLOCK) != CAN_OK)
    {
        errorCount++;
        return false;
    }

    mcp.setMode(MCP_NORMAL);

    return true;
}

// ==============================
// Mode control
// ==============================

void CANBus::setNormalMode()
{
    mcp.setMode(MCP_NORMAL);
}

void CANBus::setListenMode()
{
    mcp.setMode(MCP_LISTENONLY);
}

// ==============================
// Send frame
// ==============================

bool CANBus::send(uint32_t id, uint8_t len, const uint8_t *data)
{
    if (mcp.sendMsgBuf(id, 0, len, (uint8_t*)data) == CAN_OK)
    {
        txCount++;
        return true;
    }

    errorCount++;
    return false;
}

// ==============================
// Check for received frame
// ==============================

bool CANBus::available()
{
    if (!digitalRead(CAN_INT_PIN))
        return true;

    return false;
}

// ==============================
// Read frame
// ==============================

bool CANBus::read(CANFrame &frame)
{
    if (mcp.readMsgBuf((unsigned long*)&frame.id, &frame.length, frame.data) == CAN_OK)
    {
        rxCount++;
        return true;
    }

    errorCount++;
    return false;
}

// ==============================
// Statistics
// ==============================

uint32_t CANBus::getRxCount()
{
    return rxCount;
}

uint32_t CANBus::getTxCount()
{
    return txCount;
}

uint32_t CANBus::getErrorCount()
{
    return errorCount;
}
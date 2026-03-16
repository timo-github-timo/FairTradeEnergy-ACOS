#ifndef CAN_BUS_MOCK_H
#define CAN_BUS_MOCK_H

#include <Arduino.h>
#include <queue>

struct CANFrame
{
    uint32_t id;
    uint8_t length;
    uint8_t data[8];
};

class CANBus
{
public:

    bool begin()
    {
        Serial.println("CAN MOCK STARTED");
        return true;
    }

    bool send(uint32_t id, uint8_t len, const uint8_t *data)
    {
        CANFrame frame;
        frame.id = id;
        frame.length = len;

        memcpy(frame.data, data, len);

        rxQueue.push(frame);   // loopback
        return true;
    }

    bool available()
    {
        return !rxQueue.empty();
    }

    bool read(CANFrame &frame)
    {
        if(rxQueue.empty())
            return false;

        frame = rxQueue.front();
        rxQueue.pop();
        return true;
    }

private:

    std::queue<CANFrame> rxQueue;
};

#endif
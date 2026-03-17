#ifndef CAN_SERIAL_INJECT_H
#define CAN_SERIAL_INJECT_H

#include <Arduino.h>

// Forward declaration
class CANBus;
struct CANFrame;

class CANSerialInject
{
public:
    void begin(Stream &serial);
    void update(CANBus &can);

private:
    Stream *io;

    bool parseLine(String line, CANFrame &frame);
};

#endif
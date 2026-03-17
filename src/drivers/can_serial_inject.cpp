#include "can_serial_inject.h"

#ifdef CAN_SIMULATION
#include "can_bus_mock.h"
#else
#include "can_bus.h"
#endif

void CANSerialInject::begin(Stream &serial)
{
    io = &serial;
}

void CANSerialInject::update(CANBus &can)
{
    if (!io->available())
        return;

    String line = io->readStringUntil('\n');
    line.trim();

    CANFrame frame;

    if (parseLine(line, frame))
    {
        Serial.println("Injected CAN frame:");

        Serial.print("ID: 0x");
        Serial.println(frame.id, HEX);

        Serial.print("LEN: ");
        Serial.println(frame.length);

        Serial.print("DATA: ");
        for (int i = 0; i < frame.length; i++)
        {
            Serial.print(frame.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        // 👉 Inject into your system
        // Option A: directly process
        // Option B: simulate receive (recommended)

        // simulate receive by calling your handler
        // e.g. push into your processing logic
        // For now: just send it back onto CAN (loopback test)

        can.send(frame.id, frame.length, frame.data);
    }
}

bool CANSerialInject::parseLine(String line, CANFrame &frame)
{
    // Expected format:
    // CAN <id> <len> <data...>

    if (!line.startsWith("CAN"))
        return false;

    // Tokenize
    String tokens[12];
    int count = 0;

    while (line.length() > 0 && count < 12)
    {
        int spaceIndex = line.indexOf(' ');

        if (spaceIndex == -1)
        {
            tokens[count++] = line;
            break;
        }

        tokens[count++] = line.substring(0, spaceIndex);
        line = line.substring(spaceIndex + 1);
        line.trim();
    }

    if (count < 3)
        return false;

    // Parse ID
    frame.id = strtoul(tokens[1].c_str(), NULL, 16);

    // Parse length
    frame.length = atoi(tokens[2].c_str());

    if (frame.length > 8)
        return false;

    // Parse data
    for (int i = 0; i < frame.length; i++)
    {
        if (3 + i >= count)
            return false;

        frame.data[i] = strtoul(tokens[3 + i].c_str(), NULL, 16);
    }

    return true;
}
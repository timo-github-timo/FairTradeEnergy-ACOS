/* //    #include <Arduino.h>
//    #include "../src/drivers/can_bus_mock.h"

    // Test setup
//    void setup() {
//        Serial.begin(115200);
//        delay(1000); // Wait for serial to initialize
//
//        Serial.println("Starting CAN Bus Tests...");

        // Test 1: CANBus initialization
        Serial.println("Test 1: CANBus initialization");
        CANBus can;
        bool result = can.begin();
        if (result) {
            Serial.println("✓ CANBus.begin() returned true");
        } else {
            Serial.println("✗ CANBus.begin() returned false");
        }

        // Test 2: CANBus send and receive
        Serial.println("\nTest 2: CANBus send and receive");
        can.begin();

        uint32_t test_id = 0x123;
        uint8_t test_len = 4;
        uint8_t test_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

        bool send_result = can.send(test_id, test_len, test_data);
        if (send_result) {
            Serial.println("✓ CANBus.send() returned true");
        } else {
            Serial.println("✗ CANBus.send() returned false");
        }

        if (can.available()) {
            Serial.println("✓ Frame is available after send");
        } else {
            Serial.println("✗ Frame is not available after send");
        }

        CANFrame received_frame;
        bool read_result = can.read(received_frame);
        if (read_result) {
            Serial.println("✓ CANBus.read() returned true");

            if (received_frame.id == test_id) {
                Serial.println("✓ Frame ID matches");
            } else {
                Serial.print("✗ Frame ID mismatch: expected 0x");
                Serial.print(test_id, HEX);
                Serial.print(", got 0x");
                Serial.println(received_frame.id, HEX);
            }

            if (received_frame.length == test_len) {
                Serial.println("✓ Frame length matches");
            } else {
                Serial.print("✗ Frame length mismatch: expected ");
                Serial.print(test_len);
                Serial.print(", got ");
                Serial.println(received_frame.length);
            }

            bool data_matches = true;
            for (int i = 0; i < test_len; i++) {
                if (received_frame.data[i] != test_data[i]) {
                    data_matches = false;
                    break;
                }
            }

            if (data_matches) {
                Serial.println("✓ Frame data matches");
            } else {
                Serial.println("✗ Frame data mismatch");
            }

        } else {
            Serial.println("✗ CANBus.read() returned false");
        }

        if (!can.available()) {
            Serial.println("✓ No more frames available after read");
        } else {
            Serial.println("✗ More frames available after read");
        }

        // Test 3: CANBus counters
        Serial.println("\nTest 3: CANBus counters");
        can.begin();

        if (can.getRxCount() == 0 && can.getTxCount() == 0 && can.getErrorCount() == 0) {
            Serial.println("✓ Initial counters are 0");
        } else {
            Serial.println("✗ Initial counters are not 0");
        }

        can.send(0xABC, 2, (uint8_t[]){0x11, 0x22});

        if (can.getTxCount() == 1) {
            Serial.println("✓ TX count incremented after send");
        } else {
            Serial.print("✗ TX count not incremented: ");
            Serial.println(can.getTxCount());
        }

        can.read(received_frame);

        if (can.getRxCount() == 1) {
            Serial.println("✓ RX count incremented after read");
        } else {
            Serial.print("✗ RX count not incremented: ");
            Serial.println(can.getRxCount());
        }

        // Test 4: Multiple frames
        Serial.println("\nTest 4: Multiple frames");
        can.begin();

        for (int i = 0; i < 3; i++) {
            uint32_t id = 0x100 + i;
            uint8_t data[1] = {(uint8_t)i};
            can.send(id, 1, data);
        }

        bool all_frames_correct = true;
        for (int i = 0; i < 3; i++) {
            if (!can.available()) {
                Serial.print("✗ Frame ");
                Serial.print(i);
                Serial.println(" not available");
                all_frames_correct = false;
                break;
            }

            can.read(received_frame);
            if (received_frame.id != (uint32_t)(0x100 + i) ||
                received_frame.length != 1 ||
                received_frame.data[0] != (uint8_t)i) {
                Serial.print("✗ Frame ");
                Serial.print(i);
                Serial.println(" data incorrect");
                all_frames_correct = false;
            }
        }

        if (all_frames_correct) {
            Serial.println("✓ All frames processed correctly");
        }

        if (!can.available()) {
            Serial.println("✓ No extra frames remaining");
        } else {
            Serial.println("✗ Extra frames remaining");
        }

        Serial.println("\nAll tests completed!");
    }

    void loop() {
        // Nothing to do in loop for tests
    } */
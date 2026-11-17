
#include <Arduino.h>
#include <WiFi.h>

#include "espnow_receiver.h"
#include "command_sender.h"

static uint32_t nextCommandId = 1;

static char serialBuffer[64];
static size_t serialLength = 0;

// Replace these bytes with the actual Rocket ESP32
// station MAC address printed during its startup.
static const uint8_t rocketMAC[6] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static bool isMACConfigured() {
    for (uint8_t byte : rocketMAC) {
        if (byte != 0) {
            return true;
        }
    }

    return false;
}

static void processSerialCommand(const char* command) {
    if (strcmp(command, "LED_TEST") == 0) {
        if (!isMACConfigured()) {
            Serial.println("# Rocket MAC not configured");
            return;
        }

        uint32_t commandId = nextCommandId++;

        if (sendLedTestCommand(commandId)) {
            Serial.printf(
                "# TEST_SENT,%lu\n",
                (unsigned long)commandId
            );
        }

        return;
    }

    Serial.println("# Unknown USB command");
}

static void readSerialCommands() {
    while (Serial.available() > 0) {
        char character = Serial.read();

        if (character == '\n' || character == '\r') {
            if (serialLength == 0) {
                continue;
            }

            serialBuffer[serialLength] = '\0';

            processSerialCommand(serialBuffer);

            serialLength = 0;
            continue;
        }

        if (serialLength < sizeof(serialBuffer) - 1) {
            serialBuffer[serialLength++] = character;
        } else {
            serialLength = 0;
            Serial.println("# USB command too long");
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("# GCS TELEMETRY RECEIVER");

    if (!initReceiver()) {
        Serial.println("# Receiver initialization failed");

        while (true) {
            delay(100);
        }
    }

    if (isMACConfigured()) {
        if (!initCommandSender(rocketMAC)) {
            Serial.println("# Command sender initialization failed");
        }
    } else {
        Serial.println("# Configure Rocket MAC before sending tests");
    }
}

void loop() {
    readSerialCommands();
}

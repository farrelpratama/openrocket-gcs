
#include <Arduino.h>

#include "config.h"
#include "espnow_receiver.h"

void setup() {
    Serial.begin(GCSConfig::SERIAL_BAUD);
    Serial.setTimeout(50);

    delay(1000);

    if (!initReceiver()) {
        Serial.println("# GCS initialization FAILED");

        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');

        command.trim();
        command.toUpperCase();

        if (command == "FIRE") {
            Serial.println("# Sending LED test request...");
            sendLedTestStart();
        }

        else if (command == "OFF") {
            Serial.println("# Sending LED abort request...");
            sendLedTestAbort();
        }

        else if (command.length() > 0) {
            Serial.println("# Unknown command. Use FIRE or OFF.");
        }
    }
}

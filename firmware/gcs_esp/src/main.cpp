
#include <Arduino.h>

#include "config.h"
#include "espnow_receiver.h"

void setup() {
    Serial.begin(GCSConfig::SERIAL_BAUD);
    Serial.setTimeout(50);

    delay(1000);

    Serial.println();
    Serial.println("=== GCS TELEMETRY + COMMAND SYSTEM ===");

    if (!initReceiver()) {
        Serial.println("# Receiver initialization failed");

        while (true) {
            delay(100);
        }
    }
}

void loop() {
    processGCSInput();
}


#include <Arduino.h>
#include "espnow_receiver.h"

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
}

void loop() {
    // ESP-NOW reception is handled by the callback.
}

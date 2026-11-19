
#include "test_indicator.h"

#include <Arduino.h>

static constexpr uint8_t TEST_LED_PIN = 2;
static constexpr uint32_t TEST_DURATION_MS = 3000;

static bool active = false;
static uint32_t startTime = 0;

void initTestIndicator() {
    pinMode(TEST_LED_PIN, OUTPUT);
    digitalWrite(TEST_LED_PIN, LOW);

    active = false;
    startTime = 0;
}

void startTestIndicator() {
    if (active) {
        return;
    }

    digitalWrite(TEST_LED_PIN, HIGH);

    startTime = millis();
    active = true;

    Serial.println("[TEST] LED indicator started");
}

void updateTestIndicator() {
    if (!active) {
        return;
    }

    if (millis() - startTime >= TEST_DURATION_MS) {
        digitalWrite(TEST_LED_PIN, LOW);

        active = false;

        Serial.println("[TEST] LED indicator completed");
    }
}

bool isTestIndicatorActive() {
    return active;
}

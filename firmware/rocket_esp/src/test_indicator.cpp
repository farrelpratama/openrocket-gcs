#include "test_indicator.h"
#include "config.h"

static bool active = false;
static unsigned long startTime = 0;

bool initTestIndicator() {
    pinMode(TEST_LED_PIN, OUTPUT);
    digitalWrite(TEST_LED_PIN, LOW);
    active = false;
    startTime = 0;
    Serial.printf("[TEST] LED indicator initialized on GPIO %d\n", TEST_LED_PIN);
    return true;
}

void startTestIndicator() {
    if (active) return;
    digitalWrite(TEST_LED_PIN, HIGH);
    active = true;
    startTime = millis();
    Serial.println("[TEST] LED-only simulation started (3 seconds)");
}

void updateTestIndicator() {
    if (active && (unsigned long)(millis() - startTime) >= TEST_DURATION_MS) {
        stopTestIndicator();
        Serial.println("[TEST] LED-only simulation completed");
    }
}

bool isTestIndicatorActive() { return active; }

void stopTestIndicator() {
    digitalWrite(TEST_LED_PIN, LOW);
    active = false;
}

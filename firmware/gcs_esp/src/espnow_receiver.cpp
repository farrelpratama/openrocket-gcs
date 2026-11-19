
#include "espnow_receiver.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

#include "command_packet.h"

#define ESPNOW_CHANNEL 1
#define DEFAULT_DISPLAY_INTERVAL_MS 500

static uint32_t displayIntervalMs =
    DEFAULT_DISPLAY_INTERVAL_MS;

static uint32_t lastDisplayTime = 0;
static bool firstPacket = true;

// =====================================================
// TELEMETRY DISPLAY
// =====================================================

static void displayTelemetry(
    const TelemetryPacket& packet
) {

    Serial.println();
    Serial.println("================================");
    Serial.println("       ROCKET TELEMETRY");
    Serial.println("================================");

    Serial.printf(
        "Sequence   : %lu\n",
        static_cast<unsigned long>(packet.sequence)
    );

    Serial.printf(
        "Timestamp  : %lu us\n",
        static_cast<unsigned long>(packet.timestampUs)
    );

    Serial.println("--------------------------------");
    Serial.println("[ACCELEROMETER]");

    Serial.printf("AX : %.3f m/s2\n", packet.ax);
    Serial.printf("AY : %.3f m/s2\n", packet.ay);
    Serial.printf("AZ : %.3f m/s2\n", packet.az);

    Serial.println("--------------------------------");
    Serial.println("[GYROSCOPE]");

    Serial.printf("GX : %.3f deg/s\n", packet.gx);
    Serial.printf("GY : %.3f deg/s\n", packet.gy);
    Serial.printf("GZ : %.3f deg/s\n", packet.gz);

    Serial.println("--------------------------------");
    Serial.println("[ENVIRONMENT]");

    Serial.printf("MPU Temperature  : %.2f C\n", packet.mpuTemp);
    Serial.printf("Barometer Temp   : %.2f C\n", packet.baroTemp);
    Serial.printf("Pressure         : %.2f Pa\n", packet.pressurePa);
    Serial.printf("Pressure         : %.2f hPa\n", packet.pressureHpa);
    Serial.printf("Altitude         : %.2f m\n", packet.altitude);

    Serial.println("--------------------------------");
    Serial.println("[ORIENTATION]");

    Serial.printf("Roll  : %.2f deg\n", packet.roll);
    Serial.printf("Pitch : %.2f deg\n", packet.pitch);
    Serial.printf("Yaw   : %.2f deg\n", packet.yaw);

    Serial.println("================================");
}

// =====================================================
// ACK HANDLER
// =====================================================

static void handleAcknowledgement(
    const uint8_t* incomingData
) {

    CommandPacket response = {};

    memcpy(
        &response,
        incomingData,
        sizeof(CommandPacket)
    );

    if (!isValidCommand(response)) {
        return;
    }

    if (response.type != COMMAND_ACK) {
        return;
    }

    const char* statusText = "UNKNOWN";

    switch (response.status) {

        case STATUS_ACCEPTED:
            statusText = "ACCEPTED";
            break;

        case STATUS_COMPLETED:
            statusText = "COMPLETED";
            break;

        case STATUS_REJECTED:
            statusText = "REJECTED";
            break;
    }

    // Machine-readable output for the Tauri dashboard.
    Serial.printf(
        "#ACK,%lu,%s\n",
        static_cast<unsigned long>(response.commandId),
        statusText
    );

    // Human-readable output for Serial Monitor.
    Serial.printf(
        "[ACK] Command ID: %lu | Status: %s\n",
        static_cast<unsigned long>(response.commandId),
        statusText
    );
}

// =====================================================
// ESP-NOW RECEIVE CALLBACK
// =====================================================

static void OnDataReceived(
    const esp_now_recv_info_t* info,
    const uint8_t* incomingData,
    int len
) {

    if (info == nullptr ||
        incomingData == nullptr) {
        return;
    }

    // -------------------------------------------------
    // Check for Command ACK
    // -------------------------------------------------

    if (len == sizeof(CommandPacket)) {

        handleAcknowledgement(incomingData);

        return;
    }

    // -------------------------------------------------
    // Check for Telemetry Packet
    // -------------------------------------------------

    if (len != sizeof(TelemetryPacket)) {
        return;
    }

    TelemetryPacket packet = {};

    memcpy(
        &packet,
        incomingData,
        sizeof(TelemetryPacket)
    );

    if (packet.version != 1) {
        return;
    }

    // -------------------------------------------------
    // Telemetry Display Throttling
    // -------------------------------------------------

    uint32_t currentTime = millis();

    if (!firstPacket &&
        (uint32_t)(currentTime - lastDisplayTime)
            < displayIntervalMs) {

        return;
    }

    firstPacket = false;
    lastDisplayTime = currentTime;

    displayTelemetry(packet);
}

// =====================================================
// INITIALIZE RECEIVER
// =====================================================

bool initReceiver() {

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {

        Serial.println(
            "[ERROR] ESP-NOW initialization failed"
        );

        return false;
    }

    if (esp_now_register_recv_cb(OnDataReceived) != ESP_OK) {

        Serial.println(
            "[ERROR] ESP-NOW callback registration failed"
        );

        return false;
    }

    Serial.println("[OK] ESP-NOW receiver initialized");

    return true;
}

// =====================================================
// TELEMETRY DISPLAY INTERVAL
// =====================================================

void setTelemetryDisplayInterval(uint32_t intervalMs) {

    if (intervalMs < 50 || intervalMs > 5000) {
        return;
    }

    displayIntervalMs = intervalMs;

    Serial.printf(
        "# Telemetry display interval: %lu ms\n",
        static_cast<unsigned long>(displayIntervalMs)
    );
}

uint32_t getTelemetryDisplayInterval() {
    return displayIntervalMs;
}

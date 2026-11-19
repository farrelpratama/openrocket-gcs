
#include "espnow_receiver.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "config.h"
#include "telemetry.h"
#include "command_packet.h"

static uint32_t nextCommandId = 1;
static esp_now_peer_info_t rocketPeer{};

// ==========================================
// ESP-NOW SEND CALLBACK
// ==========================================

static void OnDataSent(
    const uint8_t* mac,
    esp_now_send_status_t status
) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("# ESP-NOW delivery: SUCCESS");
    } else {
        Serial.println("# ESP-NOW delivery: FAILED");
    }
}

// ==========================================
// ESP-NOW RECEIVE CALLBACK
// ==========================================

static void OnDataRecv(
    const esp_now_recv_info_t* info,
    const uint8_t* data,
    int len
) {
    if (data == nullptr) return;

    // --------------------------------------
    // TELEMETRY PACKET
    // --------------------------------------

    if (len == sizeof(TelemetryPacket)) {
        TelemetryPacket packet{};

        memcpy(&packet, data, sizeof(packet));

        if (packet.version != TELEMETRY_VERSION) {
            Serial.println("# Invalid telemetry version");
            return;
        }

        Serial.printf(
            "%lu,%lu,"
            "%.3f,%.3f,%.3f,"
            "%.3f,%.3f,%.3f,"
            "%.2f,"
            "%.2f,%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f\n",

            (unsigned long)packet.sequence,
            (unsigned long)packet.timestampUs,

            packet.ax,
            packet.ay,
            packet.az,

            packet.gx,
            packet.gy,
            packet.gz,

            packet.mpuTemp,

            packet.baroTemp,
            packet.pressurePa,
            packet.pressureHpa,
            packet.altitude,

            packet.roll,
            packet.pitch,
            packet.yaw
        );

        return;
    }

    // --------------------------------------
    // ACKNOWLEDGEMENT PACKET
    // --------------------------------------

    if (len == sizeof(AcknowledgementPacket)) {
        AcknowledgementPacket ack{};

        memcpy(&ack, data, sizeof(ack));

        if (ack.magic != COMMAND_MAGIC ||
            ack.version != COMMAND_PROTOCOL_VERSION) {
            Serial.println("# Invalid acknowledgement");
            return;
        }

        const char* status = "UNKNOWN";

        switch (ack.status) {
            case ACK_ACCEPTED:
                status = "ACCEPTED";
                break;

            case ACK_COMPLETED:
                status = "COMPLETED";
                break;

            case ACK_ABORTED:
                status = "ABORTED";
                break;

            case ACK_REJECTED:
                status = "REJECTED";
                break;

            case ACK_BUSY:
                status = "BUSY";
                break;
        }

        Serial.printf(
            "# LED TEST ACK | ID: %lu | STATUS: %s\n",
            (unsigned long)ack.commandId,
            status
        );

        return;
    }

    Serial.printf(
        "# Unknown packet received: %d bytes\n",
        len
    );
}

// ==========================================
// SEND COMMAND
// ==========================================

static bool sendCommand(uint8_t type) {
    CommandPacket packet{};

    packet.magic = COMMAND_MAGIC;
    packet.version = COMMAND_PROTOCOL_VERSION;
    packet.type = type;
    packet.commandId = nextCommandId++;

    esp_err_t result = esp_now_send(
        rocketPeer.peer_addr,
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet)
    );

    if (result != ESP_OK) {
        Serial.printf(
            "# Command submission failed: %d\n",
            (int)result
        );

        return false;
    }

    Serial.printf(
        "# Command submitted | ID: %lu | TYPE: %u\n",
        (unsigned long)packet.commandId,
        type
    );

    return true;
}

bool sendLedTestStart() {
    return sendCommand(COMMAND_LED_TEST_START);
}

bool sendLedTestAbort() {
    return sendCommand(COMMAND_LED_TEST_ABORT);
}

// ==========================================
// INITIALIZE GCS
// ==========================================

bool initReceiver() {
    WiFi.mode(WIFI_STA);

    delay(100);

    Serial.println();
    Serial.println("================================");
    Serial.println("       GCS ESP32 STARTUP        ");
    Serial.println("================================");

    Serial.print("# GCS MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.print("# Rocket MAC: ");

    char macString[18];

    snprintf(
        macString,
        sizeof(macString),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        GCSConfig::ROCKET_MAC[0],
        GCSConfig::ROCKET_MAC[1],
        GCSConfig::ROCKET_MAC[2],
        GCSConfig::ROCKET_MAC[3],
        GCSConfig::ROCKET_MAC[4],
        GCSConfig::ROCKET_MAC[5]
    );

    Serial.println(macString);

    // Configure channel
    if (esp_wifi_set_channel(
            GCSConfig::ESPNOW_CHANNEL,
            WIFI_SECOND_CHAN_NONE
        ) != ESP_OK) {

        Serial.println("# WiFi channel configuration failed");
        return false;
    }

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("# ESP-NOW initialization failed");
        return false;
    }

    // Register callbacks
    if (esp_now_register_recv_cb(OnDataRecv) != ESP_OK) {
        Serial.println("# Receive callback registration failed");
        return false;
    }

    if (esp_now_register_send_cb(OnDataSent) != ESP_OK) {
        Serial.println("# Send callback registration failed");
        return false;
    }

    // Validate Rocket MAC
    bool macUnset = true;

    for (uint8_t b : GCSConfig::ROCKET_MAC) {
        if (b != 0) {
            macUnset = false;
            break;
        }
    }

    if (macUnset) {
        Serial.println("# ERROR: Rocket MAC is not configured");
        return false;
    }

    // Register Rocket as peer
    memset(&rocketPeer, 0, sizeof(rocketPeer));

    memcpy(
        rocketPeer.peer_addr,
        GCSConfig::ROCKET_MAC,
        6
    );

    rocketPeer.channel = GCSConfig::ESPNOW_CHANNEL;
    rocketPeer.ifidx = WIFI_IF_STA;
    rocketPeer.encrypt = false;

    esp_err_t peerResult = esp_now_add_peer(&rocketPeer);

    if (peerResult != ESP_OK &&
        peerResult != ESP_ERR_ESPNOW_EXIST) {

        Serial.printf(
            "# Failed to add Rocket peer: %d\n",
            (int)peerResult
        );

        return false;
    }

    Serial.println("# ESP-NOW initialized successfully");
    Serial.println("# Rocket peer registered");
    Serial.println("# Telemetry receiver ready");
    Serial.println("# Command transmitter ready");

    Serial.println();
    Serial.println("# Available commands:");
    Serial.println("# FIRE = Start LED-only test");
    Serial.println("# OFF  = Abort LED-only test");

    Serial.println();
    Serial.println(
        "sequence,timestamp_us,"
        "ax,ay,az,gx,gy,gz,mpu_temp,"
        "baro_temp,pressure_pa,pressure_hpa,"
        "altitude,roll,pitch,yaw"
    );

    return true;
}

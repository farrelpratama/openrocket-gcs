#include "espnow_receiver.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "config.h"
#include "telemetry.h"
#include "command_protocol.h"

static uint32_t nextCommandId = 1;

static void printAcknowledgement(
    const AcknowledgementPacket& ack
) {
    Serial.printf(
        "# ACK id=%lu status=%u LED=%s EJECTION=%s\n",
        static_cast<unsigned long>(ack.commandId),
        ack.status,
        ack.ledActive ? "ON" : "OFF",
        ack.ejectionActive ? "ACTIVE" : "SAFE"
    );
}

static void onDataReceived(
    const esp_now_recv_info_t* info,
    const uint8_t* data,
    int len
) {
    if (info == nullptr || data == nullptr) {
        return;
    }

    static bool sourceReported = false;

    if (!sourceReported) {
        Serial.printf("# RX source=" MACSTR " len=%d\n",
                      MAC2STR(info->src_addr), len);
        sourceReported = true;
    }

    // Telemetry packet
    if (len == sizeof(TelemetryPacket)) {
        TelemetryPacket packet{};
        memcpy(&packet, data, sizeof(packet));

        if (packet.version != TELEMETRY_VERSION) {
            return;
        }

        Serial.printf(
            "%lu,%lu,"
            "%.3f,%.3f,%.3f,"
            "%.3f,%.3f,%.3f,"
            "%.2f,"
            "%.2f,%.2f,%.2f,%.2f,"
            "%.2f,%.2f,%.2f\n",

            static_cast<unsigned long>(packet.sequence),
            static_cast<unsigned long>(packet.timestampUs),

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

    // Command acknowledgement packet
    if (len == sizeof(AcknowledgementPacket)) {
        AcknowledgementPacket ack{};
        memcpy(&ack, data, sizeof(ack));

        if (ack.magic != COMMAND_MAGIC ||
            ack.version != COMMAND_VERSION) {
            return;
        }

        printAcknowledgement(ack);
    }
}

bool initReceiver() {
    WiFi.mode(WIFI_STA);

    Serial.print("# GCS MAC: ");
    Serial.println(WiFi.macAddress());

    if (esp_wifi_set_channel(
            GCSConfig::ESPNOW_CHANNEL,
            WIFI_SECOND_CHAN_NONE
        ) != ESP_OK) {
        Serial.println("# Failed to configure Wi-Fi channel");
        return false;
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("# ESP-NOW initialization failed");
        return false;
    }

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, ROCKET_MAC, 6);

    peer.channel = GCSConfig::ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    esp_err_t peerResult = esp_now_add_peer(&peer);

    if (peerResult != ESP_OK &&
        peerResult != ESP_ERR_ESPNOW_EXIST) {
        Serial.printf(
            "# Failed to add Rocket peer: %d\n",
            static_cast<int>(peerResult)
        );
        return false;
    }

    if (esp_now_register_recv_cb(onDataReceived) != ESP_OK) {
        Serial.println("# Receive callback registration failed");
        return false;
    }

    Serial.println("# GCS receiver ready");
    Serial.println("# Commands: TEST, OFF, STATUS, HELP");

    Serial.println(
        "sequence,timestamp_us,"
        "ax,ay,az,gx,gy,gz,mpu_temp,"
        "baro_temp,pressure_pa,pressure_hpa,altitude,"
        "roll,pitch,yaw"
    );

    return true;
}

bool sendRocketCommand(uint8_t command) {
    CommandPacket packet{};

    packet.magic = COMMAND_MAGIC;
    packet.version = COMMAND_VERSION;
    packet.command = command;
    packet.commandId = nextCommandId++;

    esp_err_t result = esp_now_send(
        ROCKET_MAC,
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet)
    );

    if (result != ESP_OK) {
        Serial.printf(
            "# Command send failed: %d\n",
            static_cast<int>(result)
        );
        return false;
    }

    Serial.printf(
        "# Command sent: %u (ID %lu)\n",
        command,
        static_cast<unsigned long>(packet.commandId)
    );

    return true;
}

void processGCSInput() {
    if (!Serial.available()) {
        return;
    }

    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    if (input == "TEST") {
        sendRocketCommand(CMD_TEST_START);
    }
    else if (input == "OFF") {
        sendRocketCommand(CMD_TEST_STOP);
    }
    else if (input == "STATUS") {
        sendRocketCommand(CMD_STATUS);
    }
    else if (input == "FIRE") {
        sendRocketCommand(CMD_EJECTION_TRIGGER);
    }
    else if (input == "HELP") {
        Serial.println("# TEST   - Start 3-second LED test");
        Serial.println("# FIRE   - Trigger GPIO27 ejection charge");
        Serial.println("# OFF    - Abort/cut GPIO27 and stop LED");
        Serial.println("# STATUS - Request LED status");
    }
    else if (input.length() > 0) {
        Serial.println("# Unknown command. Type HELP.");
    }
}

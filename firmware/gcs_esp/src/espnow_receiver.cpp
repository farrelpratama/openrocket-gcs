
#include "espnow_receiver.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "telemetry.h"
#include "command_packet.h"

#define ESPNOW_CHANNEL 1

static void printTelemetry(
    const TelemetryPacket& data
) {
    Serial.printf(
        "%lu,%lu,"
        "%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,"
        "%.2f,"
        "%.2f,%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%.2f\n",

        (unsigned long)data.sequence,
        (unsigned long)data.timestampUs,

        data.ax, data.ay, data.az,
        data.gx, data.gy, data.gz,

        data.mpuTemp,

        data.baroTemp,
        data.pressurePa,
        data.pressureHpa,
        data.altitude,

        data.roll,
        data.pitch,
        data.yaw
    );
}

static void OnDataRecv(
    const esp_now_recv_info_t* info,
    const uint8_t* incomingData,
    int len
) {
    if (incomingData == nullptr) {
        return;
    }

    // Handle command acknowledgement.
    if (len == sizeof(CommandPacket)) {
        CommandPacket command = {};

        memcpy(&command, incomingData, sizeof(command));

        if (!isValidCommand(command)) {
            Serial.println("# Invalid command packet");
            return;
        }

        if (command.type == COMMAND_ACK) {
            Serial.printf(
                "# TEST_ACK,%lu,%u\n",
                (unsigned long)command.commandId,
                command.status
            );
        }

        return;
    }

    // Handle ordinary telemetry.
    if (len != sizeof(TelemetryPacket)) {
        Serial.printf("# Invalid packet size: %d\n", len);
        return;
    }

    TelemetryPacket data = {};

    memcpy(&data, incomingData, sizeof(data));

    if (data.version != TELEMETRY_VERSION) {
        Serial.println("# Unsupported telemetry version");
        return;
    }

    printTelemetry(data);
}

bool initReceiver() {
    WiFi.mode(WIFI_STA);

    if (esp_wifi_set_channel(
            ESPNOW_CHANNEL,
            WIFI_SECOND_CHAN_NONE
        ) != ESP_OK) {
        Serial.println("# Failed to configure Wi-Fi channel");
        return false;
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("# ESP-NOW initialization failed");
        return false;
    }

    esp_now_register_recv_cb(OnDataRecv);

    Serial.printf(
        "# GCS MAC: %s\n",
        WiFi.macAddress().c_str()
    );

    Serial.println("# GCS receiver ready");

    Serial.println(
        "sequence,timestamp_us,"
        "ax,ay,az,gx,gy,gz,mpu_temp,"
        "baro_temp,pressure_pa,pressure_hpa,altitude,"
        "roll,pitch,yaw"
    );

    return true;
}

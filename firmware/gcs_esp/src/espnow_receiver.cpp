
#include "espnow_receiver.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <string.h>

#include "telemetry.h"

#define ESPNOW_CHANNEL 1


static void OnDataRecv(
    const esp_now_recv_info_t* esp_now_info,
    const uint8_t* incomingData,
    int len
) {
    // Validate packet size
    if (len != sizeof(TelemetryPacket)) {
        Serial.printf(
            "# Invalid packet size: %d bytes\n",
            len
        );
        return;
    }

    TelemetryPacket receivedData;

    memcpy(
        &receivedData,
        incomingData,
        sizeof(TelemetryPacket)
    );

    // Validate telemetry version
    if (receivedData.version != TELEMETRY_VERSION) {
        Serial.println("# Unsupported telemetry version");
        return;
    }

    // Print received MAC address (optional)
    if (esp_now_info != nullptr) {
        Serial.printf(
            "# Received from: %02X:%02X:%02X:%02X:%02X:%02X\n",
            esp_now_info->src_addr[0],
            esp_now_info->src_addr[1],
            esp_now_info->src_addr[2],
            esp_now_info->src_addr[3],
            esp_now_info->src_addr[4],
            esp_now_info->src_addr[5]
        );
    }

    // CSV telemetry output
    Serial.printf(
        "%lu,%lu,"
        "%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,"
        "%.2f,"
        "%.2f,%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%.2f\n",

        (unsigned long)receivedData.sequence,
        (unsigned long)receivedData.timestampUs,

        receivedData.ax,
        receivedData.ay,
        receivedData.az,

        receivedData.gx,
        receivedData.gy,
        receivedData.gz,

        receivedData.mpuTemp,

        receivedData.baroTemp,
        receivedData.pressurePa,
        receivedData.pressureHpa,
        receivedData.altitude,

        receivedData.roll,
        receivedData.pitch,
        receivedData.yaw
    );
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

    Serial.println("# GCS receiver ready");
    Serial.println(
        "sequence,timestamp_us,"
        "ax,ay,az,gx,gy,gz,mpu_temp,"
        "baro_temp,pressure_pa,pressure_hpa,altitude,"
        "roll,pitch,yaw"
    );

    return true;
}

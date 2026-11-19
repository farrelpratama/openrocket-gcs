
#include "espnow_transmitter.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#define ESPNOW_CHANNEL 1

static esp_now_peer_info_t peerInfo;

static void OnDataSent(
    const uint8_t* mac,
    esp_now_send_status_t status
) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("[ESP-NOW] Delivery success");
    } else {
        Serial.println("[ESP-NOW] Delivery failed");
    }
}

bool initTransmitter(const uint8_t* receiverMAC) {
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    // Both ESP32 boards must use the same Wi-Fi channel.
    if (esp_wifi_set_channel(
            ESPNOW_CHANNEL,
            WIFI_SECOND_CHAN_NONE
        ) != ESP_OK) {
        Serial.println("[ESP-NOW] Channel configuration failed");
        return false;
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Initialization failed");
        return false;
    }

    esp_now_register_send_cb(OnDataSent);

    memset(&peerInfo, 0, sizeof(peerInfo));

    memcpy(peerInfo.peer_addr, receiverMAC, 6);

    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[ESP-NOW] Failed to add receiver");
        return false;
    }

    Serial.println("[ESP-NOW] Transmitter ready");

    return true;
}


bool sendTelemetry(const TelemetryPacket& packet) {
    esp_err_t result = esp_now_send(
        peerInfo.peer_addr,
        (const uint8_t*)&packet,
        sizeof(TelemetryPacket)
    );

    if (result != ESP_OK) {
        Serial.printf(
            "[ESP-NOW ERROR] Send failed: %s (%d)\n",
            esp_err_to_name(result),
            result
        );

        return false;
    }

    return true;
}


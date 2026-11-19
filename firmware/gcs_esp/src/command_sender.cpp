
#include "command_sender.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

#include "command_packet.h"

#define ESPNOW_CHANNEL 1

static uint8_t rocketAddress[6] = {};
static bool senderReady = false;

bool initCommandSender(const uint8_t* rocketMAC) {

    if (rocketMAC == nullptr) {
        return false;
    }

    memcpy(rocketAddress, rocketMAC, 6);

    esp_now_peer_info_t peerInfo = {};

    memcpy(
        peerInfo.peer_addr,
        rocketAddress,
        6
    );

    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    if (esp_now_is_peer_exist(rocketAddress)) {
        esp_now_del_peer(rocketAddress);
    }

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("# Failed to add rocket command peer");
        return false;
    }

    senderReady = true;

    Serial.println("# Command sender ready");

    return true;
}

bool sendLedTestCommand(uint32_t commandId) {

    if (!senderReady) {
        Serial.println("# Command sender is not initialized");
        return false;
    }

    CommandPacket packet = {};

    packet.magic = COMMAND_MAGIC;
    packet.version = COMMAND_VERSION;
    packet.type = COMMAND_LED_TEST;
    packet.commandId = commandId;
    packet.status = STATUS_ACCEPTED;

    esp_err_t result = esp_now_send(
        rocketAddress,
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet)
    );

    if (result != ESP_OK) {

        Serial.printf(
            "# LED test send failed: %d\n",
            result
        );

        return false;
    }

    Serial.printf(
        "# LED test command queued: %lu\n",
        static_cast<unsigned long>(commandId)
    );

    return true;
}

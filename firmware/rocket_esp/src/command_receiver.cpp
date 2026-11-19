
#include "command_receiver.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "command_packet.h"
#include "test_indicator.h"

// =====================================================
// CONFIGURATION
// =====================================================

#define ESPNOW_CHANNEL 1
#define COMMAND_QUEUE_LENGTH 5

// =====================================================
// QUEUE DATA STRUCTURE
// =====================================================

struct CommandQueueItem {
    CommandPacket packet;
    uint8_t sourceMAC[6];
};

// =====================================================
// PRIVATE VARIABLES
// =====================================================

static QueueHandle_t commandQueue = nullptr;

static bool testInProgress = false;

static uint32_t activeCommandId = 0;

static uint8_t activeSourceMAC[6] = {};

// =====================================================
// REGISTER ESP-NOW PEER
// =====================================================

static bool ensurePeerRegistered(const uint8_t* mac) {

    if (mac == nullptr) {
        return false;
    }

    if (esp_now_is_peer_exist(mac)) {
        return true;
    }

    esp_now_peer_info_t peerInfo = {};

    memcpy(
        peerInfo.peer_addr,
        mac,
        6
    );

    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peerInfo);

    if (result != ESP_OK) {

        Serial.printf(
            "[COMMAND] Failed to register peer: %d\n",
            result
        );

        return false;
    }

    Serial.println("[COMMAND] Peer registered successfully");

    return true;
}

// =====================================================
// SEND ACKNOWLEDGEMENT
// =====================================================

static void sendAcknowledgement(
    const uint8_t* destinationMAC,
    uint32_t commandId,
    uint8_t status
) {

    if (!ensurePeerRegistered(destinationMAC)) {
        Serial.println("[COMMAND] Cannot send ACK: peer unavailable");
        return;
    }

    CommandPacket response = {};

    response.magic = COMMAND_MAGIC;
    response.version = COMMAND_VERSION;
    response.type = COMMAND_ACK;
    response.commandId = commandId;
    response.status = status;

    esp_err_t result = esp_now_send(
        destinationMAC,
        reinterpret_cast<const uint8_t*>(&response),
        sizeof(response)
    );

    if (result != ESP_OK) {

        Serial.printf(
            "[COMMAND] ACK send failed: %d\n",
            result
        );

        return;
    }

    const char* statusText = "UNKNOWN";

    switch (status) {

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

    Serial.printf(
        "[COMMAND] ACK queued | ID: %lu | Status: %s\n",
        static_cast<unsigned long>(commandId),
        statusText
    );
}

// =====================================================
// ESP-NOW RECEIVE CALLBACK
// =====================================================

static void OnCommandReceived(
    const esp_now_recv_info_t* info,
    const uint8_t* incomingData,
    int len
) {

    // -------------------------------------------------
    // Validate Incoming Data
    // -------------------------------------------------

    if (info == nullptr ||
        incomingData == nullptr ||
        info->src_addr == nullptr) {
        return;
    }

    if (len != sizeof(CommandPacket)) {
        return;
    }

    // -------------------------------------------------
    // Copy Incoming Packet
    // -------------------------------------------------

    CommandQueueItem item = {};

    memcpy(
        &item.packet,
        incomingData,
        sizeof(CommandPacket)
    );

    memcpy(
        item.sourceMAC,
        info->src_addr,
        sizeof(item.sourceMAC)
    );

    // -------------------------------------------------
    // Validate Packet
    // -------------------------------------------------

    if (!isValidCommand(item.packet)) {
        return;
    }

    // -------------------------------------------------
    // Validate Command Type
    // -------------------------------------------------

    // Only LED_TEST is supported in this test firmware.

    if (item.packet.type != COMMAND_LED_TEST) {
        return;
    }

    // -------------------------------------------------
    // Send Command to Queue
    // -------------------------------------------------

    if (commandQueue == nullptr) {
        return;
    }

    if (xQueueSend(commandQueue, &item, 0) != pdPASS) {

        Serial.println(
            "[COMMAND] Queue full - command dropped"
        );
    }
}

// =====================================================
// INITIALIZE COMMAND RECEIVER
// =====================================================

bool initCommandReceiver() {

    Serial.println("[COMMAND] Creating FreeRTOS queue");

    commandQueue = xQueueCreate(
        COMMAND_QUEUE_LENGTH,
        sizeof(CommandQueueItem)
    );

    if (commandQueue == nullptr) {

        Serial.println(
            "[COMMAND] Failed to create command queue"
        );

        return false;
    }

    Serial.println("[COMMAND] Registering ESP-NOW callback");

    esp_err_t result = esp_now_register_recv_cb(
        OnCommandReceived
    );

    if (result != ESP_OK) {

        Serial.printf(
            "[COMMAND] Receiver registration failed: %d\n",
            result
        );

        vQueueDelete(commandQueue);
        commandQueue = nullptr;

        return false;
    }

    Serial.println("[COMMAND] Receiver registration successful");
    Serial.println("[COMMAND] LED test receiver ready");

    return true;
}

// =====================================================
// PROCESS COMMAND RECEIVER
// =====================================================

void processCommandReceiver() {

    if (commandQueue == nullptr) {
        return;
    }

    CommandQueueItem incomingCommand = {};

    // -------------------------------------------------
    // RECEIVE COMMAND FROM QUEUE
    // -------------------------------------------------

    if (xQueueReceive(
            commandQueue,
            &incomingCommand,
            0
        ) == pdPASS) {

        Serial.println();
        Serial.println("================================");
        Serial.println("[COMMAND] NEW COMMAND RECEIVED");
        Serial.println("================================");

        Serial.printf(
            "[COMMAND] Command ID: %lu\n",
            static_cast<unsigned long>(
                incomingCommand.packet.commandId
            )
        );

        Serial.printf(
            "[COMMAND] Command Type: %u\n",
            incomingCommand.packet.type
        );

        Serial.printf(
            "[COMMAND] Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            incomingCommand.sourceMAC[0],
            incomingCommand.sourceMAC[1],
            incomingCommand.sourceMAC[2],
            incomingCommand.sourceMAC[3],
            incomingCommand.sourceMAC[4],
            incomingCommand.sourceMAC[5]
        );

        Serial.println("[COMMAND] Packet validation: PASSED");
        Serial.println("[COMMAND] Queue processing: SUCCESS");

        // -------------------------------------------------
        // CHECK EXECUTION STATE
        // -------------------------------------------------

        if (testInProgress) {

            Serial.println(
                "[COMMAND] Rejected: another LED test is active"
            );

            sendAcknowledgement(
                incomingCommand.sourceMAC,
                incomingCommand.packet.commandId,
                STATUS_REJECTED
            );

        } else {

            // -------------------------------------------------
            // STORE ACTIVE COMMAND
            // -------------------------------------------------

            activeCommandId =
                incomingCommand.packet.commandId;

            memcpy(
                activeSourceMAC,
                incomingCommand.sourceMAC,
                sizeof(activeSourceMAC)
            );

            // -------------------------------------------------
            // START LED TEST
            // -------------------------------------------------

            Serial.println("[COMMAND] Starting LED test");

            startTestIndicator();

            testInProgress = true;

            Serial.println("[COMMAND] LED test started");

            // -------------------------------------------------
            // SEND ACCEPTED ACK
            // -------------------------------------------------

            sendAcknowledgement(
                activeSourceMAC,
                activeCommandId,
                STATUS_ACCEPTED
            );

            Serial.println("[COMMAND] Status: ACCEPTED");
        }

        Serial.println("================================");
    }

    // -------------------------------------------------
    // UPDATE LED TEST
    // -------------------------------------------------

    updateTestIndicator();

    // -------------------------------------------------
    // CHECK LED TEST COMPLETION
    // -------------------------------------------------

    if (testInProgress && !isTestIndicatorActive()) {

        Serial.println();
        Serial.println("================================");
        Serial.println("[COMMAND] LED TEST COMPLETED");

        Serial.printf(
            "[COMMAND] Command ID: %lu\n",
            static_cast<unsigned long>(activeCommandId)
        );

        // -------------------------------------------------
        // SEND COMPLETED ACK
        // -------------------------------------------------

        sendAcknowledgement(
            activeSourceMAC,
            activeCommandId,
            STATUS_COMPLETED
        );

        Serial.println("[COMMAND] Status: COMPLETED");
        Serial.println("================================");

        // -------------------------------------------------
        // RESET COMMAND STATE
        // -------------------------------------------------

        testInProgress = false;
        activeCommandId = 0;

        memset(
            activeSourceMAC,
            0,
            sizeof(activeSourceMAC)
        );
    }
}

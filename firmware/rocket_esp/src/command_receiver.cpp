
#include "command_receiver.h"

#include <Arduino.h>
#include <esp_now.h>
#include <string.h>

#include "command_packet.h"
#include "espnow_transmitter.h"
#include "test_indicator.h"

static volatile bool commandPending = false;

static CommandPacket pendingCommand = {};
static uint8_t commandSourceMAC[6] = {};

static bool testInProgress = false;
static uint32_t activeCommandId = 0;

static void sendAcknowledgement(
    uint32_t commandId,
    uint8_t status
) {
    CommandPacket response = {};

    response.magic = COMMAND_MAGIC;
    response.version = COMMAND_VERSION;
    response.type = COMMAND_ACK;
    response.commandId = commandId;
    response.status = status;

    esp_err_t result = esp_now_send(
        commandSourceMAC,
        reinterpret_cast<const uint8_t*>(&response),
        sizeof(response)
    );

    if (result != ESP_OK) {
        Serial.printf(
            "[COMMAND] ACK send failed: %d\n",
            result
        );
    }
}

static void OnCommandReceived(
    const esp_now_recv_info_t* info,
    const uint8_t* incomingData,
    int len
) {
    if (info == nullptr ||
        incomingData == nullptr ||
        len != sizeof(CommandPacket)) {
        return;
    }

    CommandPacket received = {};

    memcpy(&received, incomingData, sizeof(received));

    if (!isValidCommand(received)) {
        return;
    }

    // Only accept the non-energetic LED test.
    if (received.type != COMMAND_LED_TEST) {
        return;
    }

    // Ignore another request while one is pending or active.
    if (commandPending || testInProgress) {
        return;
    }

    memcpy(
        commandSourceMAC,
        info->src_addr,
        sizeof(commandSourceMAC)
    );

    memcpy(
        &pendingCommand,
        &received,
        sizeof(pendingCommand)
    );

    commandPending = true;
}

bool initCommandReceiver() {
    if (esp_now_register_recv_cb(OnCommandReceived) != ESP_OK) {
        Serial.println("[COMMAND] Receiver registration failed");
        return false;
    }

    Serial.println("[COMMAND] LED test receiver ready");

    return true;
}

void processCommandReceiver() {
    // Process the request outside the ESP-NOW callback.
    if (commandPending && !testInProgress) {
        noInterrupts();

        CommandPacket command = pendingCommand;
        commandPending = false;

        interrupts();

        activeCommandId = command.commandId;

        startTestIndicator();
        testInProgress = true;

        sendAcknowledgement(
            activeCommandId,
            STATUS_ACCEPTED
        );

        Serial.printf(
            "[COMMAND] LED test accepted: %lu\n",
            (unsigned long)activeCommandId
        );
    }

    updateTestIndicator();

    if (testInProgress && !isTestIndicatorActive()) {
        sendAcknowledgement(
            activeCommandId,
            STATUS_COMPLETED
        );

        Serial.printf(
            "[COMMAND] LED test completed: %lu\n",
            (unsigned long)activeCommandId
        );

        testInProgress = false;
    }
}

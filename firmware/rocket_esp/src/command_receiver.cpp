#include "command_receiver.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <string.h>
#include "command_packet.h"
#include "test_indicator.h"

static portMUX_TYPE commandMux = portMUX_INITIALIZER_UNLOCKED;
static CommandPacket pendingCommand;
static uint8_t pendingSource[6];
static bool commandPending = false;
static uint8_t activeSourceMAC[6] = {0};
static uint32_t activeCommandId = 0;
static bool completionAckPending = false;

static void sendAcknowledgement(const uint8_t* mac, uint32_t id, AckStatus status) {
    AcknowledgementPacket ack{};
    ack.magic = COMMAND_MAGIC;
    ack.version = COMMAND_PROTOCOL_VERSION;
    ack.status = static_cast<uint8_t>(status);
    ack.commandId = id;
    esp_err_t result = esp_now_send(mac, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));
    if (result != ESP_OK) Serial.printf("[COMMAND] ACK send request failed: %d\n", (int)result);
}

static void OnCommandReceived(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (info == nullptr || data == nullptr || len != (int)sizeof(CommandPacket)) return;
    CommandPacket incoming{};
    memcpy(&incoming, data, sizeof(incoming));
    if (incoming.magic != COMMAND_MAGIC || incoming.version != COMMAND_PROTOCOL_VERSION) return;
    if (incoming.type != COMMAND_LED_TEST_START && incoming.type != COMMAND_LED_TEST_ABORT) return;

    portENTER_CRITICAL(&commandMux);
    if (!commandPending) {
        pendingCommand = incoming;
        memcpy(pendingSource, info->src_addr, 6);
        commandPending = true;
    }
    portEXIT_CRITICAL(&commandMux);
}

bool initCommandReceiver() {
    if (!initTestIndicator()) return false;
    esp_err_t result = esp_now_register_recv_cb(OnCommandReceived);
    if (result != ESP_OK) {
        Serial.printf("[COMMAND] Receiver registration failed: %d\n", (int)result);
        return false;
    }
    Serial.println("[COMMAND] LED-only test receiver ready");
    return true;
}

void processCommandReceiver() {
    CommandPacket cmd{};
    uint8_t source[6]{};
    bool hasCommand = false;

    portENTER_CRITICAL(&commandMux);
    if (commandPending) {
        cmd = pendingCommand;
        memcpy(source, pendingSource, 6);
        commandPending = false;
        hasCommand = true;
    }
    portEXIT_CRITICAL(&commandMux);

    if (hasCommand) {
        Serial.printf("[COMMAND] Received ID=%lu type=%u\n", (unsigned long)cmd.commandId, cmd.type);
        if (cmd.type == COMMAND_LED_TEST_START) {
            if (isTestIndicatorActive()) {
                sendAcknowledgement(source, cmd.commandId, ACK_BUSY);
                Serial.println("[COMMAND] Rejected: LED test already active");
            } else {
                memcpy(activeSourceMAC, source, 6);
                activeCommandId = cmd.commandId;
                startTestIndicator();
                sendAcknowledgement(source, cmd.commandId, ACK_ACCEPTED);
                completionAckPending = true;
            }
        } else if (cmd.type == COMMAND_LED_TEST_ABORT) {
            bool wasActive = isTestIndicatorActive();
            stopTestIndicator();
            sendAcknowledgement(source, cmd.commandId, wasActive ? ACK_ABORTED : ACK_REJECTED);
            completionAckPending = false;
            Serial.println(wasActive ? "[COMMAND] LED test aborted" : "[COMMAND] No active LED test");
        }
    }

    bool wasActive = isTestIndicatorActive();
    updateTestIndicator();
    if (wasActive && !isTestIndicatorActive() && completionAckPending) {
        sendAcknowledgement(activeSourceMAC, activeCommandId, ACK_COMPLETED);
        completionAckPending = false;
    }
}

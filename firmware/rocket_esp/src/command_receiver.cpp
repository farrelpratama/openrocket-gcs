#include "command_receiver.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "config.h"
#include "command_protocol.h"

static uint8_t gcsAddress[6];

static volatile bool commandPending = false;
static CommandPacket pendingCommand;

static bool ledActive = false;
static uint32_t ledStartTime = 0;
static bool ejectionActive = false;
static uint32_t ejectionStartTime = 0;

static void sendAcknowledgement(
    uint32_t commandId,
    AckStatus status
) {
    AcknowledgementPacket ack{};

    ack.magic = COMMAND_MAGIC;
    ack.version = COMMAND_VERSION;
    ack.status = status;
    ack.commandId = commandId;
    ack.ledActive = ledActive ? 1 : 0;
    ack.ejectionActive = ejectionActive ? 1 : 0;

    esp_err_t result = esp_now_send(
        gcsAddress,
        reinterpret_cast<const uint8_t*>(&ack),
        sizeof(ack)
    );

    if (result != ESP_OK) {
        Serial.printf(
            "[COMMAND] ACK send failed: %d\n",
            static_cast<int>(result)
        );
    }
}

static void onCommandReceived(
    const esp_now_recv_info_t* info,
    const uint8_t* data,
    int len
) {
    if (info == nullptr || data == nullptr) {
        return;
    }

    if (memcmp(info->src_addr, gcsAddress, 6) != 0) {
        return;
    }

    if (len != sizeof(CommandPacket)) {
        return;
    }

    CommandPacket received{};
    memcpy(&received, data, sizeof(received));

    if (received.magic != COMMAND_MAGIC ||
        received.version != COMMAND_VERSION) {
        return;
    }

    // One command is processed at a time.
    if (commandPending) {
        return;
    }

    memcpy(&pendingCommand, &received, sizeof(received));
    commandPending = true;
}

bool initCommandReceiver(const uint8_t* mac) {
    if (mac == nullptr) {
        return false;
    }

    memcpy(gcsAddress, mac, 6);

    pinMode(TEST_LED_PIN, OUTPUT);
    digitalWrite(TEST_LED_PIN, LOW);
    pinMode(EJECTION_PIN, OUTPUT);
    digitalWrite(EJECTION_PIN, LOW);
    ejectionActive = false;
    ejectionStartTime = 0;

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, gcsAddress, 6);

    peer.channel = ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    esp_err_t peerResult = esp_now_add_peer(&peer);

    // The transmitter may already have registered this peer.
    if (peerResult != ESP_OK &&
        peerResult != ESP_ERR_ESPNOW_EXIST) {
        Serial.printf(
            "[COMMAND] Peer registration failed: %d\n",
            static_cast<int>(peerResult)
        );
        return false;
    }

    if (esp_now_register_recv_cb(onCommandReceived) != ESP_OK) {
        Serial.println("[COMMAND] Callback registration failed");
        return false;
    }

    Serial.println("[COMMAND] Receiver ready");
    return true;
}

void processCommandReceiver() {
    // Process a received command outside the Wi-Fi callback.
    if (commandPending) {
        CommandPacket command{};

        noInterrupts();
        memcpy(&command, &pendingCommand, sizeof(command));
        commandPending = false;
        interrupts();

        switch (command.command) {
            case CMD_TEST_START:
                ledActive = true;
                ledStartTime = millis();

                digitalWrite(TEST_LED_PIN, HIGH);

                Serial.println("[TEST] LED test started");
                sendAcknowledgement(
                    command.commandId,
                    ACK_ACCEPTED
                );
                break;

            case CMD_TEST_STOP:
                ledActive = false;
                digitalWrite(TEST_LED_PIN, LOW);
                digitalWrite(EJECTION_PIN, LOW);
                ejectionActive = false;

                Serial.println("[COMMAND] Test/pyro abort executed");
                sendAcknowledgement(
                    command.commandId,
                    ACK_STOPPED
                );
                break;

            case CMD_STATUS:
                sendAcknowledgement(
                    command.commandId,
                    ACK_STATUS
                );
                break;

            case CMD_EJECTION_TRIGGER:
                Serial.println("[COMMAND] Ejection trigger activated for 3 seconds");
                digitalWrite(TEST_LED_PIN, HIGH);
                ledActive = true;
                ledStartTime = millis();
                digitalWrite(EJECTION_PIN, HIGH);
                ejectionActive = true;
                ejectionStartTime = millis();
                sendAcknowledgement(
                    command.commandId,
                    ACK_ACCEPTED
                );
                break;

            default:
                sendAcknowledgement(
                    command.commandId,
                    ACK_REJECTED
                );
                break;
        }
    }

    // Automatic 3-second LED timeout.
    if (ledActive &&
        millis() - ledStartTime >= TEST_DURATION_MS) {

        ledActive = false;
        digitalWrite(TEST_LED_PIN, LOW);

        Serial.println("[TEST] LED test completed");
    }

    if (ejectionActive &&
        millis() - ejectionStartTime >= EJECTION_PULSE_MS) {

        ejectionActive = false;
        digitalWrite(EJECTION_PIN, LOW);
        Serial.println("[COMMAND] Ejection pulse completed");
    }
}

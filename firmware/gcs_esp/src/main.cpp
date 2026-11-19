
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <cstdlib>

#include "espnow_receiver.h"
#include "command_sender.h"

// =====================================================
// ROCKET ESP32 CONFIGURATION
// =====================================================

// GANTI dengan MAC Address STA milik Rocket-ESP
const uint8_t rocketMAC[] = {
    0x08, 0xB6, 0x1F,
    0x3C, 0x13, 0xDC
};

// =====================================================
// CONFIGURATION
// =====================================================

#define SERIAL_BAUDRATE 115200
#define SERIAL_BUFFER_SIZE 64

// =====================================================
// PRIVATE VARIABLES
// =====================================================

static bool commandSenderReady = false;

static uint32_t nextCommandId = 1;

static char serialBuffer[SERIAL_BUFFER_SIZE];
static size_t serialIndex = 0;

// =====================================================
// PRINT HELP
// =====================================================

void printHelp() {
    Serial.println();
    Serial.println("========== GCS COMMAND LIST ==========");
    Serial.println("LED_TEST      : Test Rocket LED indicator");
    Serial.println("RATE <ms>     : Set telemetry display interval");
    Serial.println("STATUS        : Show GCS status");
    Serial.println("HELP          : Show command list");
    Serial.println("=====================================");
    Serial.println();
}

// =====================================================
// PRINT STATUS
// =====================================================

void printStatus() {
    Serial.println();
    Serial.println("========== GCS STATUS ==========");

    Serial.print("[STATUS] Command Sender: ");

    if (commandSenderReady) {
        Serial.println("READY");
    } else {
        Serial.println("NOT INITIALIZED");
    }

    Serial.printf(
        "[STATUS] Next Command ID: %lu\n",
        static_cast<unsigned long>(nextCommandId)
    );

    Serial.println("================================");
    Serial.println();
}

// =====================================================
// PROCESS LED TEST COMMAND
// =====================================================

void processLedTest() {

    Serial.println();
    Serial.println("[COMMAND] Sending LED_TEST");

    if (!commandSenderReady) {
        Serial.println("[ERROR] Command sender is not initialized");
        return;
    }

    uint32_t commandId = nextCommandId++;

    Serial.printf(
        "[COMMAND] Command ID: %lu\n",
        static_cast<unsigned long>(commandId)
    );

    bool success = sendLedTestCommand(commandId);

    if (success) {
        Serial.println("[SUCCESS] LED_TEST queued for ESP-NOW transmission");
        Serial.println("[INFO] Waiting for Rocket-ESP acknowledgement...");
    } else {
        Serial.println("[ERROR] LED_TEST transmission request failed");
    }

    Serial.println();
}

// =====================================================
// PROCESS SERIAL COMMAND
// =====================================================

void processSerialCommand(char* command) {

    // Remove leading spaces
    while (*command == ' ' || *command == '\t') {
        command++;
    }

    // Remove trailing spaces
    size_t length = strlen(command);

    while (length > 0 &&
           (command[length - 1] == ' ' ||
            command[length - 1] == '\t')) {

        command[--length] = '\0';
    }

    if (length == 0) {
        return;
    }

    Serial.print("[SERIAL] Received command: ");
    Serial.println(command);

    // LED TEST
    if (strcmp(command, "LED_TEST") == 0) {
        processLedTest();
        return;
    }

    // HELP
    if (strcmp(command, "HELP") == 0) {
        printHelp();
        return;
    }

    // STATUS
    if (strcmp(command, "STATUS") == 0) {
        printStatus();
        return;
    }

    // RATE
    if (strncmp(command, "RATE ", 5) == 0) {

        char* endPtr = nullptr;
        long interval = strtol(command + 5, &endPtr, 10);

        if (endPtr == command + 5 ||
            *endPtr != '\0' ||
            interval < 50 ||
            interval > 5000) {

            Serial.println(
                "[ERROR] Invalid RATE. Use RATE 50-5000"
            );

            return;
        }

        setTelemetryDisplayInterval(
            static_cast<uint32_t>(interval)
        );

        Serial.printf(
            "[SUCCESS] Telemetry display interval: %ld ms\n",
            interval
        );

        return;
    }

    Serial.println("[ERROR] Unknown command. Type HELP.");
}

// =====================================================
// READ SERIAL INPUT
// =====================================================

void readSerialInput() {

    while (Serial.available() > 0) {

        char incoming = static_cast<char>(Serial.read());

        // Newline or carriage return marks end of command
        if (incoming == '\n' || incoming == '\r') {

            if (serialIndex > 0) {

                serialBuffer[serialIndex] = '\0';

                processSerialCommand(serialBuffer);

                serialIndex = 0;
            }

            continue;
        }

        // Prevent buffer overflow
        if (serialIndex < SERIAL_BUFFER_SIZE - 1) {

            serialBuffer[serialIndex++] = incoming;

        } else {

            Serial.println("[ERROR] Serial command too long");

            serialIndex = 0;
        }
    }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

    Serial.begin(SERIAL_BAUDRATE);
    delay(1500);

    Serial.println();
    Serial.println("======================================");
    Serial.println("          GCS ESP32 SYSTEM");
    Serial.println("======================================");

    // ---------------------------------------------
    // Initialize WiFi
    // ---------------------------------------------

    Serial.println("[INIT] WiFi");

    WiFi.mode(WIFI_STA);

    delay(100);

    Serial.print("[INFO] GCS MAC Address: ");
    Serial.println(WiFi.macAddress());

    Serial.println("[OK] WiFi initialized");

    // ---------------------------------------------
    // Initialize ESP-NOW Receiver
    // ---------------------------------------------

    Serial.println();
    Serial.println("[INIT] ESP-NOW Receiver");

    if (!initReceiver()) {

        Serial.println(
            "[ERROR] ESP-NOW receiver initialization failed"
        );

        while (true) {
            delay(1000);
        }
    }

    Serial.println("[OK] ESP-NOW receiver initialized");

    // ---------------------------------------------
    // Initialize Command Sender
    // ---------------------------------------------

    Serial.println();
    Serial.println("[INIT] Command Sender");

    Serial.print("[INFO] Rocket MAC Address: ");

    for (int i = 0; i < 6; i++) {

        Serial.printf("%02X", rocketMAC[i]);

        if (i < 5) {
            Serial.print(":");
        }
    }

    Serial.println();

    commandSenderReady = initCommandSender(rocketMAC);

    if (!commandSenderReady) {

        Serial.println(
            "[ERROR] Command sender initialization failed"
        );

    } else {

        Serial.println(
            "[OK] Command sender initialized successfully"
        );
    }

    // ---------------------------------------------
    // System Ready
    // ---------------------------------------------

    Serial.println();
    Serial.println("======================================");
    Serial.println("             GCS READY");
    Serial.println("======================================");

    printHelp();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

    readSerialInput();

    delay(1);
}

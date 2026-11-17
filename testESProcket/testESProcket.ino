#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#define ESPNOW_CHANNEL 1

// MAC Address GCS (Tujuan pengiriman telemetri & penerima balasan)
uint8_t gcsAddress[] = {0xB0, 0xCB, 0xD8, 0xCE, 0x93, 0x3C};

// --- STRUKTUR TELEMETRI ---
#define TELEMETRY_VERSION 1

struct TelemetryPacket {
    uint8_t version;
    uint32_t sequence;
    uint32_t timestampUs;

    // MPU6050 Dummy
    float ax, ay, az;
    float gx, gy, gz;
    float mpuTemp;

    // Barometer Dummy
    float baroTemp;
    float pressurePa;
    float pressureHpa;
    float altitude;

    // Orientation
    float roll;
    float pitch;
    float yaw;
};

// --- STRUKTUR COMMAND ---
static constexpr uint16_t COMMAND_MAGIC = 0xC35A;
static constexpr uint8_t COMMAND_VERSION = 1;

enum CommandType : uint8_t {
    COMMAND_LED_TEST = 1,
    COMMAND_ACK = 2
};

enum CommandStatus : uint8_t {
    STATUS_ACCEPTED = 1,
    STATUS_COMPLETED = 2,
    STATUS_REJECTED = 3
};

#pragma pack(push, 1)
struct CommandPacket {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint32_t commandId;
    uint8_t status;
};
#pragma pack(pop)

static uint32_t packetSequence = 0;
static unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100; // Kirim telemetri tiap 100ms (10Hz)

// Callback saat Rocket menerima data/perintah dari GCS
static void OnDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
    if (incomingData == nullptr) return;

    if (len == sizeof(CommandPacket)) {
        CommandPacket command = {};
        memcpy(&command, incomingData, sizeof(command));

        if (command.magic == COMMAND_MAGIC && command.version == COMMAND_VERSION) {
            if (command.type == COMMAND_LED_TEST) {
                Serial.println(">> Perintah '1' diterima dari GCS!");
                Serial.println(">> Ejection Charge Works!");

                // Kirim balik respon ACK ke GCS
                CommandPacket responseAck = {};
                responseAck.magic = COMMAND_MAGIC;
                responseAck.version = COMMAND_VERSION;
                responseAck.type = COMMAND_ACK;
                responseAck.commandId = command.commandId;
                responseAck.status = STATUS_COMPLETED;

                esp_now_send(gcsAddress, reinterpret_cast<const uint8_t*>(&responseAck), sizeof(responseAck));
            }
        }
    }
}

static void OnDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
    // Status kirim telemetri
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Gagal!");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Daftarkan GCS sebagai peer ESP-NOW
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, gcsAddress, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    if (esp_now_is_peer_exist(gcsAddress)) {
        esp_now_del_peer(gcsAddress);
    }

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Gagal menambahkan GCS peer!");
        return;
    }

    Serial.println("ESP32 ROCKET SIAP (MAC: 28:56:2F:49:CE:AC).");
}

void loop() {
    unsigned long currentMillis = millis();

    // Kirim Telemetri Berkala
    if (currentMillis - lastSendTime >= sendInterval) {
        lastSendTime = currentMillis;

        TelemetryPacket data = {};
        data.version = TELEMETRY_VERSION;
        data.sequence = ++packetSequence;
        data.timestampUs = micros();

        // Data Dummy MPU6050 & Barometer
        data.ax = 0.01; data.ay = 0.02; data.az = 0.98;
        data.gx = 0.10; data.gy = 0.05; data.gz = 0.02;
        data.mpuTemp = 28.5;
        data.baroTemp = 27.2;
        data.pressurePa = 101325.0;
        data.pressureHpa = 1013.25;
        data.altitude = (currentMillis / 1000.0) * 4.5;

        data.roll = sin(currentMillis / 500.0) * 15.0;
        data.pitch = cos(currentMillis / 500.0) * 10.0;
        data.yaw = (float)((currentMillis / 100) % 360);

        esp_now_send(gcsAddress, reinterpret_cast<const uint8_t*>(&data), sizeof(data));
    }
}
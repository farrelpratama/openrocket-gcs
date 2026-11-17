#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#define ESPNOW_CHANNEL 1

// MAC Address Rocket (Tujuan pengiriman command & penerima telemetri)
uint8_t rocketAddress[] = {0x28, 0x56, 0x2F, 0x49, 0xCE, 0xAC};

// --- STRUKTUR TELEMETRI ---
#define TELEMETRY_VERSION 1

struct TelemetryPacket {
    uint8_t version;
    uint32_t sequence;
    uint32_t timestampUs;

    float ax, ay, az;
    float gx, gy, gz;
    float mpuTemp;

    float baroTemp;
    float pressurePa;
    float pressureHpa;
    float altitude;

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

// Callback saat GCS menerima data telemetri atau balasan ACK dari Rocket
static void OnDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
    if (incomingData == nullptr) return;

    // Cek apakah balasan ACK dari Rocket
    if (len == sizeof(CommandPacket)) {
        CommandPacket ackPacket = {};
        memcpy(&ackPacket, incomingData, sizeof(ackPacket));
        
        if (ackPacket.type == COMMAND_ACK) {
            Serial.printf(">> [ACK RECEIVED] Ejection Charge Works! Command ID: %lu\n", ackPacket.commandId);
        }
        return;
    }

    // Cek apakah paket telemetri
    if (len == sizeof(TelemetryPacket)) {
        TelemetryPacket tele = {};
        memcpy(&tele, incomingData, sizeof(tele));

        // Cetak data telemetri ke Serial Monitor GCS
        Serial.printf("SEQ: %lu | Alt: %.2fm | Roll: %.2f | Pitch: %.2f | Yaw: %.2f\n",
                      tele.sequence, tele.altitude, tele.roll, tele.pitch, tele.yaw);
        return;
    }
}

static void OnDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
    // Status kirim command
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Gagal di GCS!");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Daftarkan Rocket sebagai peer ESP-NOW
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, rocketAddress, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    if (esp_now_is_peer_exist(rocketAddress)) {
        esp_now_del_peer(rocketAddress);
    }

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Gagal menambahkan Rocket peer!");
        return;
    }

    Serial.println("ESP32 GCS SIAP (MAC: B0:CB:D8:CE:93:3C).");
    Serial.println("Ketik angka '1' di Serial Monitor lalu tekan ENTER untuk tes Ejection Charge.");
}

void loop() {
    // Membaca input dari Serial Monitor GCS
    if (Serial.available() > 0) {
        char inputChar = Serial.read();

        // Jika Anda mengetik "1"
        if (inputChar == '1') {
            Serial.println(">> Mengirim perintah '1' ke Rocket...");

            CommandPacket cmd = {};
            cmd.magic = COMMAND_MAGIC;
            cmd.version = COMMAND_VERSION;
            cmd.type = COMMAND_LED_TEST;
            cmd.commandId = millis();
            cmd.status = STATUS_ACCEPTED;

            // Kirim via ESP-NOW langsung ke MAC Address Rocket
            esp_err_t result = esp_now_send(rocketAddress, reinterpret_cast<const uint8_t*>(&cmd), sizeof(cmd));

            if (result == ESP_OK) {
                Serial.println(">> Perintah berhasil dikirim ke Rocket!");
            } else {
                Serial.println(">> Gagal mengirim perintah.");
            }
        }
    }
}
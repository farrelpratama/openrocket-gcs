#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

// Pake ESP yang Micro USB punya GMAT

uint8_t receiverMAC[] = {0xB0, 0xCB, 0xD8, 0xCE, 0x93, 0x3C};

#define SDA_PIN 21
#define SCL_PIN 22
#define MPU_ADDR 0x68

typedef struct struct_message {
  float ax, ay, az;
  float gx, gy, gz;
  float temp;
} struct_message;
struct_message myData;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

bool mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);          // PWR_MGMT_1
  Wire.write(0x00);          // wake up
  return Wire.endTransmission() == 0;
}

bool mpuRead() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 14) != 14) return false;

  int16_t rax = (Wire.read() << 8) | Wire.read();
  int16_t ray = (Wire.read() << 8) | Wire.read();
  int16_t raz = (Wire.read() << 8) | Wire.read();
  int16_t rt  = (Wire.read() << 8) | Wire.read();
  int16_t rgx = (Wire.read() << 8) | Wire.read();
  int16_t rgy = (Wire.read() << 8) | Wire.read();
  int16_t rgz = (Wire.read() << 8) | Wire.read();

  myData.ax = rax / 16384.0 * 9.80665;
  myData.ay = ray / 16384.0 * 9.80665;
  myData.az = raz / 16384.0 * 9.80665;
  myData.gx = rgx / 131.0 * DEG_TO_RAD;
  myData.gy = rgy / 131.0 * DEG_TO_RAD;
  myData.gz = rgz / 131.0 * DEG_TO_RAD;
  myData.temp = rt / 340.0 + 36.53;
  return true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!mpuInit()) {
    Serial.println("MPU6050 not found, check wiring");
    while (1) delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // Biar ga Brownout, tadi "E BOD: Brownout detector was triggered".
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed"); return; }
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) { Serial.println("Failed to add peer"); return; }
}

void loop() {
  if (mpuRead()) {
    esp_now_send(receiverMAC, (uint8_t *)&myData, sizeof(myData));
  } else {
    Serial.println("MPU read failed");
  }
  delay(100);
}
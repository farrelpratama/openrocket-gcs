#include <esp_now.h>
#include <WiFi.h>

// Pake ESP yang Type C USB punya Tajib

typedef struct struct_message {
  float ax, ay, az;
  float gx, gy, gz;
  float temp;
} struct_message;
struct_message myData;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Acc: %.2f %.2f %.2f | Gyro: %.2f %.2f %.2f | Temp: %.1f\n",
                myData.ax, myData.ay, myData.az,
                myData.gx, myData.gy, myData.gz, myData.temp);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed"); return; }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("Receiver ready, waiting for data...");
}

void loop() {}
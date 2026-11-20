#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "config.h"
#include "pins.h"

#include "MPU6050.h"
#include "BMP180.h"
#include "orientation.h"

#include "telemetry.h"
#include "espnow_transmitter.h"
#include "command_receiver.h"

// GCS ESP32 MAC address
const uint8_t receiverMAC[] = {
    0xB0, 0xCB, 0xD8, 0xCE, 0x93, 0x3C
};

// Sensor objects
MPU6050 mpu;
BMP180 barometer;
Orientation orientation;

// Sensor initialization state
bool mpuReady = false;
bool barometerReady = false;

// Telemetry
TelemetryPacket telemetry;

uint32_t sequenceNumber = 0;
uint32_t previousTime = 0;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    Serial.println();
    Serial.println("=== ROCKET TELEMETRY SYSTEM ===");

    // Set the ejection output to SAFE before initializing any peripherals.
    pinMode(EJECTION_PIN, OUTPUT);
    digitalWrite(EJECTION_PIN, LOW);
    Serial.println("[SAFETY] Ejection GPIO27 set LOW at boot");

    // Initialize I2C
    Serial.println("[INIT] I2C");

    Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize MPU6050. Sensor failure must not block ejection commands.
    Serial.println("[INIT] MPU6050");

    mpuReady = mpu.begin();
    if (!mpuReady) {
        Serial.println("[ERROR] MPU6050 initialization failed");
    } else {
        Serial.println("[CALIBRATION] Gyroscope");

        if (!mpu.calibrateGyroscope()) {
            Serial.println("[ERROR] Gyroscope calibration failed");
            mpuReady = false;
        }
    }

    // Initialize BMP180
    Serial.println("[INIT] BMP180");

    barometerReady = barometer.begin();
    if (!barometerReady) {
        Serial.println("[ERROR] BMP180 initialization failed");
    } else {
        Serial.println("[CALIBRATION] Ground pressure");

        if (!barometer.calibrateGroundPressure(
                BMP180_GROUND_SAMPLES
            )) {
            Serial.println("[ERROR] Barometer calibration failed");
            barometerReady = false;
        }
    }

    orientation.reset();

    // Initialize ESP-NOW transmitter
    Serial.println("[INIT] ESP-NOW");

    if (!initTransmitter(receiverMAC)) {
        Serial.println("[ERROR] Transmitter initialization failed");
    }

    // Print Rocket ESP32 MAC address
    Serial.print("[INFO] Rocket ESP32 MAC: ");
    Serial.println(WiFi.macAddress());

    // Initialize command receiver even if sensor telemetry is unavailable.
    if (!initCommandReceiver(receiverMAC)) {
        Serial.println("[ERROR] Command receiver initialization failed");
    }

    previousTime = micros();

    Serial.println("=== ROCKET SYSTEM READY ===");
}

void loop() {
    // Process incoming GCS commands first, even if a sensor read fails.
    processCommandReceiver();

    if (!mpuReady) {
        // Send an alive telemetry frame so the GCS can diagnose sensor/peer state.
        telemetry.version = TELEMETRY_VERSION;
        telemetry.sequence = sequenceNumber++;
        telemetry.timestampUs = micros();

        // Keep all sensor and orientation fields at their initialized zero values.
        if (!sendTelemetry(telemetry)) {
            Serial.println("[ESP-NOW] Telemetry send request failed");
        }

        delay(10);
        return;
    }

    SensorData imuData{};
    BarometerData baroData{};

    // Read sensors independently. A sensor failure must not stop ESP-NOW
    // telemetry: unavailable values are sent as zero below.
    bool imuReadOk = false;
    if (mpuReady) {
        if (!mpu.readSensor(imuData)) {
            Serial.println("[ERROR] MPU6050 read failed");
        } else {
            imuReadOk = true;
        }
    }

    if (barometerReady && !barometer.readSensor(baroData)) {
        Serial.println("[ERROR] BMP180 read failed");
        barometerReady = false;
    }

    // Calculate elapsed time
    uint32_t currentTime = micros();

    float deltaTime =
        (currentTime - previousTime) / 1000000.0f;

    if (deltaTime <= 0.0f) {
        deltaTime = 0.000001f;
    }

    previousTime = currentTime;

    // Calculate orientation only when the IMU was successfully read.
    OrientationData angle{};
    if (imuReadOk) {
        angle = orientation.calculate(imuData, deltaTime);
    }

    // Build telemetry packet
    telemetry.version = TELEMETRY_VERSION;

    telemetry.sequence = sequenceNumber++;
    telemetry.timestampUs = currentTime;

    // MPU6050 data
    telemetry.ax = imuData.ax;
    telemetry.ay = imuData.ay;
    telemetry.az = imuData.az;

    telemetry.gx = imuData.gx;
    telemetry.gy = imuData.gy;
    telemetry.gz = imuData.gz;

    telemetry.mpuTemp = imuData.temperature;

    // BMP180 data
    if (barometerReady) {
        telemetry.baroTemp = baroData.temperatureC;
        telemetry.pressurePa = baroData.pressurePa;
        telemetry.pressureHpa = baroData.pressureHpa;
        telemetry.altitude = baroData.altitudeM;
    } else {
        telemetry.baroTemp = 0.0f;
        telemetry.pressurePa = 0.0f;
        telemetry.pressureHpa = 0.0f;
        telemetry.altitude = 0.0f;
    }

    // Orientation data
    telemetry.roll = angle.roll;
    telemetry.pitch = angle.pitch;
    telemetry.yaw = angle.yaw;

    // Transmit telemetry
    if (!sendTelemetry(telemetry)) {
        Serial.println("[ESP-NOW] Telemetry send request failed");
    }

    delay(1);
}

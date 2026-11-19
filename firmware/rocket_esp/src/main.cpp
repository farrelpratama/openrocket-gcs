
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

// Telemetry
TelemetryPacket telemetry;

uint32_t sequenceNumber = 0;
uint32_t previousTime = 0;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);

    Serial.println();
    Serial.println("=== ROCKET TELEMETRY SYSTEM ===");

    // Initialize I2C
    Serial.println("[INIT] I2C");

    Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize MPU6050
    Serial.println("[INIT] MPU6050");

    if (!mpu.begin()) {
        Serial.println("[ERROR] MPU6050 initialization failed");

        while (true) {
            delay(100);
        }
    }

    // Initialize BMP180
    Serial.println("[INIT] BMP180");

    if (!barometer.begin()) {
        Serial.println("[ERROR] BMP180 initialization failed");

        while (true) {
            delay(100);
        }
    }

    // Calibrate gyroscope while the rocket is stationary
    Serial.println("[CALIBRATION] Gyroscope");

    if (!mpu.calibrateGyroscope()) {
        Serial.println("[ERROR] Gyroscope calibration failed");

        while (true) {
            delay(100);
        }
    }

    // Calibrate ground pressure using configured sample count
    Serial.println("[CALIBRATION] Ground pressure");

    if (!barometer.calibrateGroundPressure(
            BMP180_GROUND_SAMPLES
        )) {
        Serial.println("[ERROR] Barometer calibration failed");

        while (true) {
            delay(100);
        }
    }

    // Reset orientation calculations
    orientation.reset();

    // Initialize ESP-NOW transmitter
    Serial.println("[INIT] ESP-NOW");

    if (!initTransmitter(receiverMAC)) {
        Serial.println("[ERROR] Transmitter initialization failed");

        while (true) {
            delay(100);
        }
    }

    // Print Rocket ESP32 MAC address
    Serial.print("[INFO] Rocket ESP32 MAC: ");
    Serial.println(WiFi.macAddress());

    // Initialize command receiver.
    // receiverMAC is the GCS ESP32 MAC address.
    if (!initCommandReceiver(receiverMAC)) {
        Serial.println("[ERROR] Command receiver initialization failed");

        while (true) {
            delay(100);
        }
    }

    previousTime = micros();

    Serial.println("=== ROCKET SYSTEM READY ===");
}

void loop() {
    // Process incoming GCS commands and update LED test
    processCommandReceiver();

    SensorData imuData;
    BarometerData baroData;

    // Read MPU6050
    if (!mpu.readSensor(imuData)) {
        Serial.println("[ERROR] MPU6050 read failed");
        delay(10);
        return;
    }

    // Read BMP180
    if (!barometer.readSensor(baroData)) {
        Serial.println("[ERROR] BMP180 read failed");
        delay(10);
        return;
    }

    // Calculate elapsed time
    uint32_t currentTime = micros();

    float deltaTime =
        (currentTime - previousTime) / 1000000.0f;

    previousTime = currentTime;

    // Calculate orientation
    OrientationData angle =
        orientation.calculate(imuData, deltaTime);

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
    telemetry.baroTemp = baroData.temperatureC;
    telemetry.pressurePa = baroData.pressurePa;
    telemetry.pressureHpa = baroData.pressureHpa;
    telemetry.altitude = baroData.altitudeM;

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

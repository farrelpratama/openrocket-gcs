
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "pins.h"

#include "MPU6050.h"
#include "BMP180.h"
#include "orientation.h"

#include "telemetry.h"
#include "espnow_transmitter.h"

// Command receiver
#include "command_receiver.h"
#include "test_indicator.h"

// =====================================================
// ESP-NOW CONFIGURATION
// =====================================================

// GCS ESP32 MAC address
const uint8_t receiverMAC[] = {
    0xB0, 0xCB, 0xD8,
    0xCE, 0x93, 0x3C
};

// Telemetry transmission interval: 50 ms = 20 Hz
const uint32_t SEND_INTERVAL_MS = 50;

// =====================================================
// SENSOR OBJECTS
// =====================================================

MPU6050 mpu;
BMP180 barometer;
Orientation orientation;

// =====================================================
// TELEMETRY
// =====================================================

TelemetryPacket telemetry;

uint32_t sequenceNumber = 0;
uint32_t previousTime = 0;

uint32_t lastSendTime = 0;

// =====================================================
// SETUP
// =====================================================

void setup() {

    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("    ROCKET TELEMETRY SYSTEM");
    Serial.println("================================");

    // -------------------------------------------------
    // Initialize I2C
    // -------------------------------------------------

    Serial.println("[INIT] I2C");

    Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
    Wire.setClock(I2C_CLOCK_SPEED);

    // -------------------------------------------------
    // Initialize MPU6050
    // -------------------------------------------------

    Serial.println("[INIT] MPU6050");

    if (!mpu.begin()) {

        Serial.println("[ERROR] MPU6050 initialization failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("[OK] MPU6050 initialized");

    // -------------------------------------------------
    // Initialize BMP180
    // -------------------------------------------------

    Serial.println("[INIT] BMP180");

    if (!barometer.begin()) {

        Serial.println("[ERROR] BMP180 initialization failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("[OK] BMP180 initialized");

    // -------------------------------------------------
    // Gyroscope Calibration
    // -------------------------------------------------

    Serial.println("[CALIBRATION] Gyroscope");
    Serial.println("[INFO] Keep rocket stationary");

    if (!mpu.calibrateGyroscope()) {

        Serial.println("[ERROR] Gyroscope calibration failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("[OK] Gyroscope calibrated");

    // -------------------------------------------------
    // Ground Pressure Calibration
    // -------------------------------------------------

    Serial.println("[CALIBRATION] Ground pressure");

    if (!barometer.calibrateGroundPressure(
            BMP180_GROUND_SAMPLES
        )) {

        Serial.println("[ERROR] Barometer calibration failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("[OK] Ground pressure calibrated");

    // -------------------------------------------------
    // Initialize Orientation
    // -------------------------------------------------

    orientation.reset();

    Serial.println("[OK] Orientation initialized");

    // -------------------------------------------------
    // Initialize ESP-NOW
    // -------------------------------------------------

    Serial.println("[INIT] ESP-NOW");

    if (!initTransmitter(receiverMAC)) {

        Serial.println("[ERROR] Transmitter initialization failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("[OK] ESP-NOW transmitter initialized");

    // -------------------------------------------------
    // Initialize LED Test Indicator
    // -------------------------------------------------

    Serial.println("[INIT] Test Indicator");

    initTestIndicator();

    Serial.println("[OK] Test Indicator initialized");

    // -------------------------------------------------
    // Initialize Command Receiver
    // -------------------------------------------------

    Serial.println("[INIT] Command Receiver");

    if (!initCommandReceiver()) {

        Serial.println("[ERROR] Command receiver initialization failed");

        while (true) {
            delay(100);
        }
    }

    Serial.println("[OK] Command receiver initialized");

    // -------------------------------------------------
    // Initialize Timing
    // -------------------------------------------------

    previousTime = micros();
    lastSendTime = millis();

    Serial.println();
    Serial.println("================================");
    Serial.println("      ROCKET SYSTEM READY");
    Serial.println("================================");

    Serial.printf(
        "[INFO] Telemetry rate: %lu Hz\n",
        1000UL / SEND_INTERVAL_MS
    );

    Serial.println("[INFO] Command receiver: READY");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

    // -------------------------------------------------
    // Process Incoming Commands
    // -------------------------------------------------

    // Handle LED test commands and update the indicator.
    // This must run continuously, even if a sensor read fails.

    processCommandReceiver();

    // -------------------------------------------------
    // Sensor Data Objects
    // -------------------------------------------------

    SensorData imuData;
    BarometerData baroData;

    // -------------------------------------------------
    // Read MPU6050
    // -------------------------------------------------

    if (!mpu.readSensor(imuData)) {

        Serial.println("[ERROR] MPU6050 read failed");

        delay(10);
        return;
    }

    // -------------------------------------------------
    // Read BMP180
    // -------------------------------------------------

    if (!barometer.readSensor(baroData)) {

        Serial.println("[ERROR] BMP180 read failed");

        delay(10);
        return;
    }

    // -------------------------------------------------
    // Calculate Delta Time
    // -------------------------------------------------

    uint32_t currentTime = micros();

    float deltaTime =
        (currentTime - previousTime) / 1000000.0f;

    previousTime = currentTime;

    // -------------------------------------------------
    // Calculate Orientation
    // -------------------------------------------------

    OrientationData angle =
        orientation.calculate(imuData, deltaTime);

    // -------------------------------------------------
    // Build Telemetry Packet
    // -------------------------------------------------

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

    // -------------------------------------------------
    // ESP-NOW Transmission (20 Hz)
    // -------------------------------------------------

    uint32_t currentMillis = millis();

    if (currentMillis - lastSendTime >= SEND_INTERVAL_MS) {

        lastSendTime = currentMillis;

        if (!sendTelemetry(telemetry)) {

            Serial.println(
                "[ESP-NOW] Send request skipped or failed"
            );
        }
    }

    // -------------------------------------------------
    // Small Delay
    // -------------------------------------------------

    delay(1);
}

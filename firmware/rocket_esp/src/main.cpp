
#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "pins.h"
#include "MPU6050.h"
#include "orientation.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

MPU6050 mpu;
Orientation orientation;

// Timing
uint32_t previousTime = 0;

// ============================================
// SETUP
// ============================================

void setup() {

    Serial.begin(115200);

    delay(1000);

    Serial.println("\n================================");
    Serial.println("       ROCKET FLIGHT COMPUTER");
    Serial.println("================================");

    // Initialize I2C
    Wire.begin(
        MPU_SDA_PIN,
        MPU_SCL_PIN
    );

    Wire.setClock(I2C_CLOCK_SPEED);

    // Initialize MPU6050
    if (!mpu.begin()) {

        Serial.println(
            "[FATAL] MPU6050 initialization failed."
        );

        while (true) {
            delay(1000);
        }
    }

    // Calibrate gyroscope
    if (!mpu.calibrateGyroscope()) {

        Serial.println(
            "[FATAL] Gyroscope calibration failed."
        );

        while (true) {
            delay(1000);
        }
    }

    // Reset orientation
    orientation.reset();

    previousTime = micros();

    Serial.println("\n================================");
    Serial.println("   ORIENTATION SYSTEM READY");
    Serial.println("================================");

    Serial.println(
        "Roll | Pitch | Yaw"
    );
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {

    SensorData sensor;

    // Read sensor
    if (!mpu.readSensor(sensor)) {

        Serial.println(
            "[ERROR] Failed to read sensor."
        );

        return;
    }

    // Calculate elapsed time
    uint32_t currentTime = micros();

    float deltaTime =
        (currentTime - previousTime) / 1000000.0f;

    previousTime = currentTime;

    // Calculate orientation
    OrientationData result =
        orientation.calculate(
            sensor,
            deltaTime
        );

    // Display results
    Serial.printf(
        "ROLL: %7.2f | PITCH: %7.2f | YAW: %7.2f\n",
        result.roll,
        result.pitch,
        result.yaw
    );

    delay(SAMPLE_INTERVAL_MS);
}

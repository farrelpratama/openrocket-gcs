#include "MPU6050.h"
#include "pins.h"

#include <Wire.h>

namespace {
constexpr float radiansToDegrees = 57.2957795f;
}

bool MPU6050::begin() {
    Serial.println("[IMU] Initializing I2C...");
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);

    Serial.println("[IMU] Detecting MPU6050...");
    if (!mpu.begin(0x68, &Wire)) {
        Serial.println("[IMU] ERROR: MPU6050 not detected!");
        initialized = false;
        return false;
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    initialized = true;
    Serial.println("[IMU] MPU6050 detected!");
    Serial.println("[IMU] Configuration complete.");
    return true;
}

bool MPU6050::read(MPU6050Data &data) {
    if (!initialized) {
        data.valid = false;
        return false;
    }

    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temperature;

    if (!mpu.getEvent(&accel, &gyro, &temperature)) {
        data.valid = false;
        return false;
    }

    data.ax = accel.acceleration.x;
    data.ay = accel.acceleration.y;
    data.az = accel.acceleration.z;
    data.gx = gyro.gyro.x * radiansToDegrees;
    data.gy = gyro.gyro.y * radiansToDegrees;
    data.gz = gyro.gyro.z * radiansToDegrees;
    data.timestamp = millis();
    data.valid = true;
    return true;
}

bool MPU6050::isReady() const {
    return initialized;
}

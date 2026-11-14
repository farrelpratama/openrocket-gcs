#include <Arduino.h>

#include "config.h"
#include "MPU6050.h"
#include "orientation.h"

MPU6050 mpu;
MPU6050Data mpuData;
Orientation orientation;

uint32_t lastSampleTime = 0;

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    delay(500);

    Serial.println();
    Serial.println("================================");
    Serial.println("     ROCKET AVIONICS SYSTEM");
    Serial.println("================================");
    Serial.print("Device: ");
    Serial.println(DEVICE_NAME);
    Serial.print("Firmware: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.println();

    if (!mpu.begin()) {
        Serial.println("[SYSTEM] IMU initialization failed!");
        return;
    }

    orientation.begin();
    Serial.println("[SYSTEM] Orientation initialized.");
}

void loop() {
    const uint32_t currentTime = millis();
    if (currentTime - lastSampleTime < IMU_SAMPLE_INTERVAL_MS) {
        return;
    }

    lastSampleTime = currentTime;
    if (!mpu.read(mpuData)) {
        Serial.println("[IMU] Sensor read failed!");
        return;
    }

    orientation.update(mpuData);
    const OrientationData orientationData = orientation.getData();
    Serial.printf(
        "TIME: %lu ms | "
        "ACC: X=%.2f Y=%.2f Z=%.2f | "
        "GYRO: X=%.2f Y=%.2f Z=%.2f | "
        "ROLL: %.2f | PITCH: %.2f | YAW: %.2f\n",
        static_cast<unsigned long>(mpuData.timestamp),
        mpuData.ax,
        mpuData.ay,
        mpuData.az,
        mpuData.gx,
        mpuData.gy,
        mpuData.gz,
        orientationData.roll,
        orientationData.pitch,
        orientationData.yaw
    );
}

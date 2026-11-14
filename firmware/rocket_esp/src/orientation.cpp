#include "orientation.h"
#include <math.h>

constexpr float radiansToDegrees = 57.2957795f;

void Orientation::begin() {
    roll = 0.0f;
    pitch = 0.0f;
    yaw = 0.0f;

    previousTime = millis();
}

void Orientation::update(const MPU6050Data &mpuData) {
    if (!mpuData.valid) return;

    uint32_t currentTime = mpuData.timestamp;

    float dt = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;

    if (dt <= 0.0f || dt > 0.5f) return;

    // Accelerometer angle estimation
    float rollAcc = atan2f(
        mpuData.ay,
        mpuData.az
    ) * radiansToDegrees;

    float pitchAcc = atan2f(
        -mpuData.ax,
        sqrtf(
            mpuData.ay * mpuData.ay +
            mpuData.az * mpuData.az
        )
    ) * radiansToDegrees;

    // Complementary filter
    roll = alpha * (roll + mpuData.gx * dt)
         + (1.0f - alpha) * rollAcc;

    pitch = alpha * (pitch + mpuData.gy * dt)
          + (1.0f - alpha) * pitchAcc;

    // Gyroscope integration for yaw
    yaw += mpuData.gz * dt;
}

OrientationData Orientation::getData() const {
    return {
        roll,
        pitch,
        yaw
    };
}
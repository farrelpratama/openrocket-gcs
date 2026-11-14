
#include "orientation.h"

#include <Arduino.h>
#include <math.h>

Orientation::Orientation() {

    yawAngle = 0.0f;
}

void Orientation::reset() {

    yawAngle = 0.0f;
}

OrientationData Orientation::calculate(
    const SensorData& sensor,
    float deltaTime
) {

    OrientationData result;

    // ============================================
    // ROLL
    // ============================================

    result.roll = atan2f(
        sensor.ay,
        sensor.az
    ) * RAD_TO_DEG;

    // ============================================
    // PITCH
    // ============================================

    result.pitch = atan2f(
        -sensor.ax,
        sqrtf(
            sensor.ay * sensor.ay +
            sensor.az * sensor.az
        )
    ) * RAD_TO_DEG;

    // ============================================
    // YAW
    // ============================================

    yawAngle += sensor.gz * deltaTime;

    // Normalize yaw to -180 ... +180 degrees
    if (yawAngle > 180.0f) {
        yawAngle -= 360.0f;
    }

    if (yawAngle < -180.0f) {
        yawAngle += 360.0f;
    }

    result.yaw = yawAngle;

    return result;
}

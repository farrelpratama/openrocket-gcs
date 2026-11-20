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
    // MPU6050 dipasang vertikal dengan sumbu X sejajar badan roket.
    // ay dan az menentukan rotasi mengelilingi sumbu longitudinal X.

    result.roll = atan2f(
        sensor.ay,
        sensor.az
    ) * RAD_TO_DEG;

    // ============================================
    // PITCH RELATIVE TO VERTICAL
    // ============================================
    // Saat roket tegak, sumbu X membaca gaya gravitasi.
    // Sudut pitch dihitung sebagai kemiringan terhadap garis vertikal.

    result.pitch = atan2f(
        sqrtf(
            sensor.ay * sensor.ay +
            sensor.az * sensor.az
        ),
        sensor.ax
    ) * RAD_TO_DEG;

    // ============================================
    // YAW / HEADING
    // ============================================
    // Integrasi gyroscope X karena sumbu X menjadi poros utama roket.

    yawAngle += sensor.gx * deltaTime;

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

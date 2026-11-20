
#ifndef ORIENTATION_H
#define ORIENTATION_H

#include "MPU6050.h"

struct OrientationData {
    float roll;
    float pitch;
    float yaw;
};

class Orientation {

public:
    Orientation();

    void reset();

    OrientationData calculate(
        const SensorData& sensor,
        float deltaTime
    );

private:
    float yawAngle;
};

#endif

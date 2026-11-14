#ifndef ORIENTATION_H
#define ORIENTATION_H

#include <Arduino.h>
#include "MPU6050.h"

struct OrientationData {
    float roll;
    float pitch;
    float yaw;
};

class Orientation {
public:
    void begin();
    void update(const MPU6050Data &mpuData);
    OrientationData getData() const;

private:
    float roll = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;

    float alpha = 0.98f;
    uint32_t previousTime = 0;
};

#endif
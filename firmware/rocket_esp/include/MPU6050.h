
#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

struct MPU6050Data {
    // Accelerometer (m/s^2)
    float ax;
    float ay;
    float az;

    // Gyroscope (degrees/second)
    float gx;
    float gy;
    float gz;

    // Timestamp
    uint32_t timestamp;

    // Data validity
    bool valid;
};

class MPU6050 {
public:
    bool begin();
    bool read(MPU6050Data &data);
    bool isReady() const;

private:
    Adafruit_MPU6050 mpu;

    bool initialized = false;
};

#endif

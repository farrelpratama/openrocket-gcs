
#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>

struct SensorData {
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float temperature;
};

struct GyroCalibrationData {
    float meanX;
    float meanY;
    float meanZ;

    float stdX;
    float stdY;
    float stdZ;

    int samples;
};

class MPU6050 {

public:
    MPU6050();

    bool begin();

    bool readSensor(SensorData& data);

    bool calibrateGyroscope();

    GyroCalibrationData getCalibrationData() const;

private:
    float gyroOffsetX;
    float gyroOffsetY;
    float gyroOffsetZ;

    float gyroStdX;
    float gyroStdY;
    float gyroStdZ;

    bool writeRegister(uint8_t reg, uint8_t value);

    bool readRegister(uint8_t reg, uint8_t& value);

    bool readRawData(
        int16_t& ax,
        int16_t& ay,
        int16_t& az,
        int16_t& temperature,
        int16_t& gx,
        int16_t& gy,
        int16_t& gz
    );
};

#endif

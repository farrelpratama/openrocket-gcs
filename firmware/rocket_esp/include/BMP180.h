
#ifndef BMP180_H
#define BMP180_H

#include <Arduino.h>

struct BarometerData {
    float temperatureC;
    float pressurePa;
    float pressureHpa;
    float altitudeM;
};

class BMP180 {

public:
    BMP180();

    bool begin();

    bool readSensor(BarometerData& data);

    bool calibrateGroundPressure(
        uint8_t samples = 20
    );

    float getGroundPressure() const;

private:
    // BMP180 calibration coefficients
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;

    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;

    int16_t B1;
    int16_t B2;
    int16_t MB;
    int16_t MC;
    int16_t MD;

    // Reference pressure
    float groundPressure;

    // Internal calculations
    bool readCalibrationData();

    bool readInt16(
        uint8_t reg,
        int16_t& value
    );

    bool readUInt16(
        uint8_t reg,
        uint16_t& value
    );

    bool writeRegister(
        uint8_t reg,
        uint8_t value
    );

    bool readRawTemperature(uint16_t& ut);

    bool readRawPressure(uint32_t& up);

    bool calculateTemperature(
        uint16_t ut,
        int32_t& b5,
        float& temperature
    );

    bool calculatePressure(
        uint32_t up,
        int32_t b5,
        uint32_t& pressure
    );

    float calculateAltitude(
        float pressurePa
    ) const;
};

#endif

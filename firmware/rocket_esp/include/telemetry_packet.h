#pragma once
#include <Arduino.h>

#pragma pack(push, 1)
struct TelemetryPacket {
    uint8_t version;
    uint32_t sequence;
    uint32_t timestampUs;

    float ax, ay, az;       // m/s^2
    float gx, gy, gz;       // degrees/s
    float mpuTemp;          // Celsius

    float baroTemp;         // Celsius
    float pressurePa;       // Pa
    float pressureHpa;      // hPa

    float altitude;         // meters, relative to ground calibration
    float roll;             // degrees
    float pitch;            // degrees
    float yaw;              // degrees (gyro-integrated; drifts without magnetometer)

    uint16_t crc;
};
#pragma pack(pop)

constexpr uint8_t TELEMETRY_VERSION = 1;

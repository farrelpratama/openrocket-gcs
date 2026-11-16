
#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>

#define TELEMETRY_VERSION 1

struct TelemetryPacket {
    uint8_t version;

    uint32_t sequence;
    uint32_t timestampUs;

    // MPU6050
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float mpuTemp;

    // BMP180
    float baroTemp;
    float pressurePa;
    float pressureHpa;
    float altitude;

    // Orientation
    float roll;
    float pitch;
    float yaw;
};

static_assert(sizeof(TelemetryPacket) <= 250,
              "Telemetry packet exceeds ESP-NOW v1 payload limit");

#endif

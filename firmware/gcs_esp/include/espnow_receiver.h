
#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

#include <Arduino.h>

struct TelemetryPacket {
    uint8_t version;

    uint32_t sequence;
    uint32_t timestampUs;

    float ax, ay, az;
    float gx, gy, gz;

    float mpuTemp;
    float baroTemp;
    float pressurePa;
    float pressureHpa;
    float altitude;

    float roll;
    float pitch;
    float yaw;
};

bool initReceiver();

void setTelemetryDisplayInterval(uint32_t intervalMs);

uint32_t getTelemetryDisplayInterval();

#endif

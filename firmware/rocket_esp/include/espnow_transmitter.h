
#ifndef ESPNOW_TRANSMITTER_H
#define ESPNOW_TRANSMITTER_H

#include <stdint.h>
#include "telemetry.h"

bool initTransmitter(const uint8_t* receiverMAC);
bool sendTelemetry(const TelemetryPacket& packet);

#endif


#pragma once

#include <Arduino.h>

namespace GCSConfig {
    constexpr uint8_t ESPNOW_CHANNEL = 1;
    constexpr uint32_t SERIAL_BAUD = 115200;
}

// Replace these six bytes with the Rocket ESP32's actual STA MAC.
static const uint8_t ROCKET_MAC[6] = {
    0x08, 0xB6, 0x1F, 0x3C, 0x13, 0xDC
};

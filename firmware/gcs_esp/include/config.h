
#pragma once

#include <Arduino.h>

namespace GCSConfig {
    constexpr uint8_t ESPNOW_CHANNEL = 1;
    constexpr uint32_t SERIAL_BAUD = 115200;
}

// Replace these six bytes with the Rocket ESP32's actual STA MAC.
static const uint8_t ROCKET_MAC[6] = {
    0x30, 0x76, 0xF5, 0xF3, 0xA0, 0x9C
};

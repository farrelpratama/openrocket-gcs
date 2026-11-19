#pragma once
#include <Arduino.h>
namespace GCSConfig {
constexpr uint8_t ESPNOW_CHANNEL=1;
constexpr uint32_t SERIAL_BAUD=115200;
constexpr uint32_t CSV_OUTPUT_INTERVAL_MS=50;
}

// Replace with the Rocket ESP32 MAC printed during rocket startup.
// Example format: {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}
namespace GCSConfig {
constexpr uint8_t ROCKET_MAC[6] = {0x08, 0xB6, 0x1F, 0x3C, 0x13, 0xDC};
}

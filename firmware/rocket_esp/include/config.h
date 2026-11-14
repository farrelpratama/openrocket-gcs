
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// System configuration
#define DEVICE_NAME "ROCKET_ESP"
#define FIRMWARE_VERSION "0.1.0"

// Serial configuration
#define SERIAL_BAUDRATE 115200

// IMU configuration
#define IMU_SAMPLE_RATE_HZ 10
#define IMU_SAMPLE_INTERVAL_MS (1000 / IMU_SAMPLE_RATE_HZ)

// Sensor configuration
#define IMU_ACCEL_RANGE 8
#define IMU_GYRO_RANGE 500

#endif

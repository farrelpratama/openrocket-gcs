
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// ESP-NOW CONFIGURATION
// ============================================================

#define ESPNOW_CHANNEL 1
#define TELEMETRY_INTERVAL_MS 50
#define SERIAL_BAUD_RATE 115200

// ============================================================
// I2C CONFIGURATION
// ============================================================

#define I2C_CLOCK_SPEED 400000

// ============================================================
// MPU6050 CONFIGURATION
// ============================================================

#define MPU6050_ADDRESS 0x68

// Accelerometer: +/- 8g
#define ACCEL_RANGE_G 8.0f
#define ACCEL_SENSITIVITY 4096.0f

// Gyroscope: +/- 500 degrees/second
#define GYRO_RANGE_DPS 500.0f
#define GYRO_SENSITIVITY 65.5f

// Gyroscope calibration
#define GYRO_CALIBRATION_SAMPLES 2000
#define GYRO_CALIBRATION_DELAY 5
#define GYRO_STD_WARNING 0.5f

// Standard gravitational acceleration
#define GRAVITY 9.80665f

// ============================================================
// BMP180 CONFIGURATION
// ============================================================

#define BMP180_ADDRESS 0x77
#define BMP180_CHIP_ID 0x55

// Oversampling: Ultra High Resolution
#define BMP180_OVERSAMPLING 3

// Ground pressure calibration
#define BMP180_GROUND_SAMPLES 100

// Altitude filter
#define ALTITUDE_FILTER_ALPHA 0.2f

// ============================================================
// LED TEST CONFIGURATION
// ============================================================

#define TEST_LED_PIN 2
#define TEST_DURATION_MS 3000UL
#define EJECTION_PIN 27
#define EJECTION_PULSE_MS 3000UL

#endif

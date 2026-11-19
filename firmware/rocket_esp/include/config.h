
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ESP-NOW
#define ESPNOW_CHANNEL 1
#define TELEMETRY_INTERVAL_MS 50
#define SERIAL_BAUD_RATE 115200

// I2C
#define I2C_CLOCK_SPEED 400000

// Sensor addresses
#define MPU6050_ADDRESS 0x68
#define BMP180_ADDRESS 0x77
#define BMP180_CHIP_ID 0x55

// MPU6050 configuration
// Accelerometer: +/- 8g
#define ACCEL_SENSITIVITY 4096.0f

// Gyroscope: +/- 500 degrees/second
#define GYRO_SENSITIVITY 65.5f

#define GRAVITY 9.80665f

// Gyroscope calibration
#define GYRO_CALIBRATION_SAMPLES 1000
#define GYRO_CALIBRATION_DELAY 5
#define GYRO_STD_WARNING 0.5f

// BMP180 configuration
#define BMP180_OVERSAMPLING 3
#define BMP180_GROUND_SAMPLES 100

// Altitude filter
#define ALTITUDE_FILTER_ALPHA 0.2f

// LED-only test simulation
#define TEST_LED_PIN 2
#define TEST_DURATION_MS 3000UL

#endif


#ifndef CONFIG_H
#define CONFIG_H

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

// ============================================================
// BMP180 CONFIGURATION
// ============================================================

#define BMP180_ADDRESS 0x77
#define BMP180_CHIP_ID 0x55

// Oversampling setting
// 0 = Ultra Low Power
// 1 = Standard
// 2 = High Resolution
// 3 = Ultra High Resolution
#define BMP180_OVERSAMPLING 1

// Ground pressure calibration
#define BMP180_GROUND_SAMPLES 20

// ============================================================
// I2C CONFIGURATION
// ============================================================

#define I2C_CLOCK_SPEED 400000

// ============================================================
// SYSTEM CONFIGURATION
// ============================================================

// Main loop interval (milliseconds)
#define SAMPLE_INTERVAL_MS 10

// Standard gravitational acceleration (m/s²)
#define GRAVITY 9.80665f

#endif

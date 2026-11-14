
#ifndef CONFIG_H
#define CONFIG_H

// MPU6050 I2C address
#define MPU6050_ADDRESS 0x68

// I2C configuration
#define I2C_CLOCK_SPEED 400000

// Accelerometer configuration: +/- 8g
#define ACCEL_RANGE_G 8.0f
#define ACCEL_SENSITIVITY 4096.0f

// Gyroscope configuration: +/- 500 degrees/second
#define GYRO_RANGE_DPS 500.0f
#define GYRO_SENSITIVITY 65.5f

// Calibration configuration
#define GYRO_CALIBRATION_SAMPLES 2000
#define GYRO_CALIBRATION_DELAY 5

// Calibration noise warning threshold
#define GYRO_STD_WARNING 0.5f

// Main loop interval
#define SAMPLE_INTERVAL_MS 10

// Standard gravity
#define GRAVITY 9.80665f

#endif

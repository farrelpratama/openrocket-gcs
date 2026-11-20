
#include "MPU6050.h"
#include "config.h"

#include <Wire.h>
#include <math.h>

// ============================================================
// MPU6050 REGISTERS
// ============================================================

#define REG_SMPLRT_DIV   0x19
#define REG_CONFIG       0x1A
#define REG_GYRO_CONFIG  0x1B
#define REG_ACCEL_CONFIG 0x1C
#define REG_ACCEL_XOUT_H 0x3B
#define REG_PWR_MGMT_1   0x6B
#define REG_WHO_AM_I     0x75

// ============================================================
// RUNNING STATISTICS - WELFORD ALGORITHM
// ============================================================

struct RunningStats {

    int count = 0;

    double mean = 0.0;
    double M2 = 0.0;

    void update(double value) {

        count++;

        double delta = value - mean;

        mean += delta / count;

        double delta2 = value - mean;

        M2 += delta * delta2;
    }

    double getMean() const {
        return mean;
    }

    double getStdDev() const {

        if (count == 0) {
            return 0.0;
        }

        return sqrt(M2 / count);
    }
};

// ============================================================
// CONSTRUCTOR
// ============================================================

MPU6050::MPU6050() {

    gyroOffsetX = 0.0f;
    gyroOffsetY = 0.0f;
    gyroOffsetZ = 0.0f;

    gyroStdX = 0.0f;
    gyroStdY = 0.0f;
    gyroStdZ = 0.0f;
}

// ============================================================
// WRITE REGISTER
// ============================================================

bool MPU6050::writeRegister(uint8_t reg, uint8_t value) {

    Wire.beginTransmission(MPU6050_ADDRESS);

    Wire.write(reg);
    Wire.write(value);

    return Wire.endTransmission() == 0;
}

// ============================================================
// READ REGISTER
// ============================================================

bool MPU6050::readRegister(uint8_t reg, uint8_t& value) {

    Wire.beginTransmission(MPU6050_ADDRESS);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(
        static_cast<uint8_t>(MPU6050_ADDRESS),
        static_cast<uint8_t>(1)
    ) != 1) {
        return false;
    }

    value = Wire.read();

    return true;
}

// ============================================================
// INITIALIZATION
// ============================================================

bool MPU6050::begin() {

    Serial.println("\n[MPU6050] Initializing...");

    uint8_t whoAmI = 0;

    if (!readRegister(REG_WHO_AM_I, whoAmI)) {

        Serial.println(
            "[MPU6050] WHO_AM_I communication failed."
        );

        return false;
    }

    Serial.printf(
        "[MPU6050] WHO_AM_I: 0x%02X\n",
        whoAmI
    );

    // Experimental mode:
    // Continue despite unexpected identity value.
    if (whoAmI != 0x68) {

        Serial.println(
            "[WARNING] Unexpected device identity."
        );

        Serial.println(
            "[WARNING] Continuing in experimental mode."
        );
    }

    // Wake up sensor
    if (!writeRegister(REG_PWR_MGMT_1, 0x00)) {
        return false;
    }

    delay(100);

    // Configure Digital Low Pass Filter
    if (!writeRegister(REG_CONFIG, 0x03)) {
        return false;
    }

    // Sample rate: 200 Hz
    if (!writeRegister(REG_SMPLRT_DIV, 0x04)) {
        return false;
    }

    // Accelerometer: +/- 8g
    if (!writeRegister(REG_ACCEL_CONFIG, 0x10)) {
        return false;
    }

    // Gyroscope: +/- 500 degrees/second
    if (!writeRegister(REG_GYRO_CONFIG, 0x08)) {
        return false;
    }

    // Verify configuration registers
    uint8_t accelConfig;
    uint8_t gyroConfig;

    if (!readRegister(REG_ACCEL_CONFIG, accelConfig)) {
        return false;
    }

    if (!readRegister(REG_GYRO_CONFIG, gyroConfig)) {
        return false;
    }

    if ((accelConfig & 0x18) != 0x10) {

        Serial.println(
            "[MPU6050] Accelerometer configuration failed."
        );

        return false;
    }

    if ((gyroConfig & 0x18) != 0x08) {

        Serial.println(
            "[MPU6050] Gyroscope configuration failed."
        );

        return false;
    }

    Serial.println(
        "[MPU6050] Initialization successful."
    );

    return true;
}

// ============================================================
// RAW SENSOR ACQUISITION
// ============================================================

bool MPU6050::readRawData(
    int16_t& ax,
    int16_t& ay,
    int16_t& az,
    int16_t& temperature,
    int16_t& gx,
    int16_t& gy,
    int16_t& gz
) {

    Wire.beginTransmission(MPU6050_ADDRESS);

    Wire.write(REG_ACCEL_XOUT_H);

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    uint8_t received = Wire.requestFrom(
        static_cast<uint8_t>(MPU6050_ADDRESS),
        static_cast<uint8_t>(14)
    );

    if (received != 14) {

        while (Wire.available()) {
            Wire.read();
        }

        return false;
    }

    // Helper: combine two bytes into signed 16-bit value
    auto readInt16 = []() -> int16_t {

        uint16_t high = Wire.read();
        uint16_t low = Wire.read();

        return static_cast<int16_t>(
            (high << 8) | low
        );
    };

    ax = readInt16();
    ay = readInt16();
    az = readInt16();

    temperature = readInt16();

    gx = readInt16();
    gy = readInt16();
    gz = readInt16();

    return true;
}

// ============================================================
// GYROSCOPE CALIBRATION
// ============================================================

bool MPU6050::calibrateGyroscope() {

    Serial.println("\n================================");
    Serial.println("     GYROSCOPE CALIBRATION");
    Serial.println("================================");

    Serial.printf(
        "Samples: %d\n",
        GYRO_CALIBRATION_SAMPLES
    );

    Serial.println(
        "Keep the sensor completely stationary."
    );

    // Countdown
    for (int i = 3; i > 0; i--) {

        Serial.printf("Starting in %d...\n", i);

        delay(1000);
    }

    int16_t ax, ay, az;
    int16_t temperature;
    int16_t gx, gy, gz;

    // Discard initial readings
    for (int i = 0; i < 20; i++) {

        if (!readRawData(
            ax, ay, az,
            temperature,
            gx, gy, gz
        )) {
            Serial.println(
                "[ERROR] Sensor warm-up failed."
            );

            return false;
        }

        delay(5);
    }

    RunningStats statsX;
    RunningStats statsY;
    RunningStats statsZ;

    int validSamples = 0;

    Serial.println("[CALIBRATION] Collecting samples...");

    while (validSamples < GYRO_CALIBRATION_SAMPLES) {

        if (!readRawData(
            ax, ay, az,
            temperature,
            gx, gy, gz
        )) {

            Serial.println(
                "[ERROR] Sensor read failed during calibration."
            );

            return false;
        }

        float x = gx / GYRO_SENSITIVITY;
        float y = gy / GYRO_SENSITIVITY;
        float z = gz / GYRO_SENSITIVITY;

        statsX.update(x);
        statsY.update(y);
        statsZ.update(z);

        validSamples++;

        if (validSamples % 100 == 0) {

            Serial.printf(
                "[CALIBRATION] %d / %d\n",
                validSamples,
                GYRO_CALIBRATION_SAMPLES
            );
        }

        delay(GYRO_CALIBRATION_DELAY);
    }

    // Store calibration offsets
    gyroOffsetX = statsX.getMean();
    gyroOffsetY = statsY.getMean();
    gyroOffsetZ = statsZ.getMean();

    // Store standard deviations
    gyroStdX = statsX.getStdDev();
    gyroStdY = statsY.getStdDev();
    gyroStdZ = statsZ.getStdDev();

    // Calibration report
    Serial.println("\n================================");
    Serial.println("      CALIBRATION RESULTS");
    Serial.println("================================");

    Serial.println("\nGyroscope Bias (deg/s)");

    Serial.printf("X: %.4f\n", gyroOffsetX);
    Serial.printf("Y: %.4f\n", gyroOffsetY);
    Serial.printf("Z: %.4f\n", gyroOffsetZ);

    Serial.println("\nStandard Deviation (deg/s)");

    Serial.printf("X: %.4f\n", gyroStdX);
    Serial.printf("Y: %.4f\n", gyroStdY);
    Serial.printf("Z: %.4f\n", gyroStdZ);

    if (
        gyroStdX > GYRO_STD_WARNING ||
        gyroStdY > GYRO_STD_WARNING ||
        gyroStdZ > GYRO_STD_WARNING
    ) {

        Serial.println(
            "\n[WARNING] High gyroscope noise detected."
        );

    } else {

        Serial.println(
            "\n[SUCCESS] Calibration completed."
        );
    }

    Serial.println("================================");

    return true;
}

// ============================================================
// GET CALIBRATION DATA
// ============================================================

GyroCalibrationData MPU6050::getCalibrationData() const {

    GyroCalibrationData data;

    data.meanX = gyroOffsetX;
    data.meanY = gyroOffsetY;
    data.meanZ = gyroOffsetZ;

    data.stdX = gyroStdX;
    data.stdY = gyroStdY;
    data.stdZ = gyroStdZ;

    data.samples = GYRO_CALIBRATION_SAMPLES;

    return data;
}

// ============================================================
// READ CONVERTED SENSOR DATA
// ============================================================

bool MPU6050::readSensor(SensorData& data) {

    int16_t ax, ay, az;
    int16_t temperature;
    int16_t gx, gy, gz;

    if (!readRawData(
        ax, ay, az,
        temperature,
        gx, gy, gz
    )) {
        return false;
    }

    // Accelerometer conversion: raw -> m/s²
    data.ax =
        (static_cast<float>(ax) / ACCEL_SENSITIVITY)
        * GRAVITY;

    data.ay =
        (static_cast<float>(ay) / ACCEL_SENSITIVITY)
        * GRAVITY;

    data.az =
        (static_cast<float>(az) / ACCEL_SENSITIVITY)
        * GRAVITY;

    // Gyroscope conversion: raw -> degrees/second
    // Remove calibrated bias
    data.gx =
        (static_cast<float>(gx) / GYRO_SENSITIVITY)
        - gyroOffsetX;

    data.gy =
        (static_cast<float>(gy) / GYRO_SENSITIVITY)
        - gyroOffsetY;

    data.gz =
        (static_cast<float>(gz) / GYRO_SENSITIVITY)
        - gyroOffsetZ;

    // Temperature conversion
    data.temperature =
        (static_cast<float>(temperature) / 340.0f)
        + 36.53f;

    return true;
}

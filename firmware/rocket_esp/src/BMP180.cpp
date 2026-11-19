
#include "BMP180.h"
#include "config.h"

#include <Wire.h>
#include <math.h>

// ============================================================
// BMP180 REGISTERS
// ============================================================

#define REG_CHIP_ID       0xD0
#define REG_CONTROL       0xF4
#define REG_DATA_MSB      0xF6

#define REG_AC1           0xAA
#define REG_AC2           0xAC
#define REG_AC3           0xAE
#define REG_AC4           0xB0
#define REG_AC5           0xB2
#define REG_AC6           0xB4
#define REG_B1            0xB6
#define REG_B2            0xB8
#define REG_MB            0xBA
#define REG_MC            0xBC
#define REG_MD            0xBE

// ============================================================
// FILTER CONFIGURATION
// ============================================================

// Lower alpha = smoother output, but slower response.
// Higher alpha = faster response, but more noise.
static constexpr float ALTITUDE_FILTER_ALPHA = 0.2f;

// ============================================================
// CONSTRUCTOR
// ============================================================

BMP180::BMP180() {
    AC1 = 0;
    AC2 = 0;
    AC3 = 0;

    AC4 = 0;
    AC5 = 0;
    AC6 = 0;

    B1 = 0;
    B2 = 0;
    MB = 0;
    MC = 0;
    MD = 0;

    groundPressure = 0.0f;

    filteredAltitude = 0.0f;
    altitudeFilterInitialized = false;

    altitudeOffset = 0.0f;
    altitudeBaselineInitialized = false;
}

// ============================================================
// WRITE REGISTER
// ============================================================

bool BMP180::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(BMP180_ADDRESS);

    Wire.write(reg);
    Wire.write(value);

    return Wire.endTransmission() == 0;
}

// ============================================================
// READ SIGNED 16-BIT REGISTER
// ============================================================

bool BMP180::readInt16(uint8_t reg, int16_t& value) {
    Wire.beginTransmission(BMP180_ADDRESS);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(
            static_cast<uint8_t>(BMP180_ADDRESS),
            static_cast<uint8_t>(2)
        ) != 2) {
        return false;
    }

    uint16_t raw =
        (static_cast<uint16_t>(Wire.read()) << 8) |
        static_cast<uint16_t>(Wire.read());

    value = static_cast<int16_t>(raw);

    return true;
}

// ============================================================
// READ UNSIGNED 16-BIT REGISTER
// ============================================================

bool BMP180::readUInt16(uint8_t reg, uint16_t& value) {
    Wire.beginTransmission(BMP180_ADDRESS);
    Wire.write(reg);

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(
            static_cast<uint8_t>(BMP180_ADDRESS),
            static_cast<uint8_t>(2)
        ) != 2) {
        return false;
    }

    value =
        (static_cast<uint16_t>(Wire.read()) << 8) |
        static_cast<uint16_t>(Wire.read());

    return true;
}

// ============================================================
// READ FACTORY CALIBRATION COEFFICIENTS
// ============================================================

bool BMP180::readCalibrationData() {
    if (!readInt16(REG_AC1, AC1)) return false;
    if (!readInt16(REG_AC2, AC2)) return false;
    if (!readInt16(REG_AC3, AC3)) return false;

    if (!readUInt16(REG_AC4, AC4)) return false;
    if (!readUInt16(REG_AC5, AC5)) return false;
    if (!readUInt16(REG_AC6, AC6)) return false;

    if (!readInt16(REG_B1, B1)) return false;
    if (!readInt16(REG_B2, B2)) return false;

    if (!readInt16(REG_MB, MB)) return false;
    if (!readInt16(REG_MC, MC)) return false;
    if (!readInt16(REG_MD, MD)) return false;

    // Basic calibration data validation
    if (
        AC4 == 0 || AC4 == 0xFFFF ||
        AC5 == 0 || AC5 == 0xFFFF ||
        AC6 == 0 || AC6 == 0xFFFF
    ) {
        Serial.println(
            "[BMP180] Invalid calibration coefficients."
        );

        return false;
    }

    return true;
}

// ============================================================
// INITIALIZATION
// ============================================================

bool BMP180::begin() {
    Serial.println("\n[BMP180] Initializing...");

    uint8_t chipID = 0;

    Wire.beginTransmission(BMP180_ADDRESS);
    Wire.write(REG_CHIP_ID);

    if (Wire.endTransmission(false) != 0) {
        Serial.println(
            "[BMP180] I2C communication failed."
        );

        return false;
    }

    if (Wire.requestFrom(
            static_cast<uint8_t>(BMP180_ADDRESS),
            static_cast<uint8_t>(1)
        ) != 1) {
        Serial.println(
            "[BMP180] Chip ID read failed."
        );

        return false;
    }

    chipID = Wire.read();

    Serial.printf(
        "[BMP180] CHIP ID: 0x%02X\n",
        chipID
    );

    if (chipID != BMP180_CHIP_ID) {
        Serial.println(
            "[BMP180] Unexpected chip identity."
        );

        return false;
    }

    if (!readCalibrationData()) {
        Serial.println(
            "[BMP180] Calibration data loading failed."
        );

        return false;
    }

    Serial.println(
        "[BMP180] Factory calibration loaded."
    );

    Serial.println(
        "[BMP180] Initialization successful."
    );

    return true;
}

// ============================================================
// READ RAW TEMPERATURE
// ============================================================

bool BMP180::readRawTemperature(uint16_t& ut) {
    if (!writeRegister(REG_CONTROL, 0x2E)) {
        return false;
    }

    // Maximum conversion time: 4.5 ms
    delay(5);

    uint16_t raw = 0;

    if (!readUInt16(REG_DATA_MSB, raw)) {
        return false;
    }

    ut = raw;

    return true;
}

// ============================================================
// READ RAW PRESSURE
// ============================================================

bool BMP180::readRawPressure(uint32_t& up) {
    uint8_t command =
        0x34 + (BMP180_OVERSAMPLING << 6);

    if (!writeRegister(REG_CONTROL, command)) {
        return false;
    }

    // Conversion time depends on oversampling
#if BMP180_OVERSAMPLING == 0
    delay(5);
#elif BMP180_OVERSAMPLING == 1
    delay(8);
#elif BMP180_OVERSAMPLING == 2
    delay(14);
#elif BMP180_OVERSAMPLING == 3
    delay(26);
#else
#error "Invalid BMP180 oversampling setting"
#endif

    Wire.beginTransmission(BMP180_ADDRESS);
    Wire.write(REG_DATA_MSB);

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (Wire.requestFrom(
            static_cast<uint8_t>(BMP180_ADDRESS),
            static_cast<uint8_t>(3)
        ) != 3) {
        return false;
    }

    uint32_t msb = Wire.read();
    uint32_t lsb = Wire.read();
    uint32_t xlsb = Wire.read();

    uint32_t raw =
        (msb << 16) |
        (lsb << 8) |
        xlsb;

    up = raw >> (8 - BMP180_OVERSAMPLING);

    return true;
}

// ============================================================
// TEMPERATURE COMPENSATION
// ============================================================

bool BMP180::calculateTemperature(
    uint16_t ut,
    int32_t& b5,
    float& temperature
) {
    int64_t x1 =
        ((static_cast<int64_t>(ut) - AC6) * AC5) >> 15;

    int64_t denominator = x1 + MD;

    if (denominator == 0) {
        return false;
    }

    int64_t x2 =
        (static_cast<int64_t>(MC) << 11) /
        denominator;

    b5 = static_cast<int32_t>(x1 + x2);

    int32_t tempRaw = (b5 + 8) >> 4;

    temperature = tempRaw / 10.0f;

    return true;
}

// ============================================================
// PRESSURE COMPENSATION
// ============================================================

bool BMP180::calculatePressure(
    uint32_t up,
    int32_t b5,
    uint32_t& pressure
) {
    int64_t b6 = static_cast<int64_t>(b5) - 4000;

    int64_t b6Squared = (b6 * b6) >> 12;

    int64_t x1 =
        (static_cast<int64_t>(B2) * b6Squared) >> 11;

    int64_t x2 =
        (static_cast<int64_t>(AC2) * b6) >> 11;

    int64_t x3 = x1 + x2;

    int64_t b3 =
        ((((static_cast<int64_t>(AC1) * 4 + x3)
        << BMP180_OVERSAMPLING) + 2) >> 2);

    x1 =
        (static_cast<int64_t>(AC3) * b6) >> 13;

    x2 =
        (static_cast<int64_t>(B1) * b6Squared) >> 16;

    x3 = (x1 + x2 + 2) >> 2;

    int64_t b4Factor = x3 + 32768;

    if (b4Factor <= 0) {
        return false;
    }

    uint64_t b4 =
        (static_cast<uint64_t>(AC4) *
        static_cast<uint64_t>(b4Factor)) >> 15;

    if (b4 == 0 || static_cast<int64_t>(up) <= b3) {
        return false;
    }

    uint64_t b7 =
        (static_cast<uint64_t>(
            static_cast<int64_t>(up) - b3
        )) * (50000ULL >> BMP180_OVERSAMPLING);

    int64_t p;

    if (b7 < 0x80000000ULL) {
        p = static_cast<int64_t>((b7 * 2) / b4);
    } else {
        p = static_cast<int64_t>((b7 / b4) * 2);
    }

    int64_t pressureSquared = (p >> 8) * (p >> 8);

    x1 = (pressureSquared * 3038) >> 16;

    x2 = (-7357LL * p) >> 16;

    p += (x1 + x2 + 3791) >> 4;

    if (p <= 0 || p > UINT32_MAX) {
        return false;
    }

    pressure = static_cast<uint32_t>(p);

    return true;
}

// ============================================================
// ALTITUDE CALCULATION
// ============================================================

float BMP180::calculateAltitude(float pressurePa) const {
    if (groundPressure <= 0.0f || pressurePa <= 0.0f) {
        return 0.0f;
    }

    return 44330.0f * (
        1.0f - powf(
            pressurePa / groundPressure,
            0.19029495f
        )
    );
}

// ============================================================
// RESET ALTITUDE FILTER AND BASELINE
// ============================================================

void BMP180::resetAltitudeFilter() {
    filteredAltitude = 0.0f;
    altitudeFilterInitialized = false;

    altitudeOffset = 0.0f;
    altitudeBaselineInitialized = false;
}

// ============================================================
// READ SENSOR
// ============================================================

bool BMP180::readSensor(BarometerData& data) {
    uint16_t ut = 0;
    uint32_t up = 0;

    int32_t b5 = 0;

    float temperature = 0.0f;
    uint32_t pressure = 0;

    // Read raw temperature
    if (!readRawTemperature(ut)) {
        return false;
    }

    // Calculate compensated temperature
    if (!calculateTemperature(
            ut,
            b5,
            temperature
        )) {
        return false;
    }

    // Read raw pressure
    if (!readRawPressure(up)) {
        return false;
    }

    // Calculate compensated pressure
    if (!calculatePressure(
            up,
            b5,
            pressure
        )) {
        return false;
    }

    // Store sensor data
    data.temperatureC = temperature;

    data.pressurePa =
        static_cast<float>(pressure);

    data.pressureHpa =
        data.pressurePa / 100.0f;

    // Calculate altitude relative to ground pressure
    float calculatedAltitude =
        calculateAltitude(data.pressurePa);

    // Establish zero-altitude baseline on first reading
    // after ground calibration.
    if (
        groundPressure > 0.0f &&
        !altitudeBaselineInitialized
    ) {
        altitudeOffset = calculatedAltitude;
        altitudeBaselineInitialized = true;

        Serial.println(
            "[BMP180] Relative altitude baseline initialized."
        );
    }

    // Raw altitude with baseline correction, without filtering
    data.altitudeRawM =
        calculatedAltitude - altitudeOffset;

    // Backward compatibility
    data.altitudeM = data.altitudeRawM;

    // Low-pass filter
    if (!altitudeFilterInitialized) {
        filteredAltitude = data.altitudeRawM;
        altitudeFilterInitialized = true;
    } else {
        filteredAltitude =
            (ALTITUDE_FILTER_ALPHA * data.altitudeRawM) +
            ((1.0f - ALTITUDE_FILTER_ALPHA) * filteredAltitude);
    }

    data.altitudeFilteredM = filteredAltitude;

    return true;
}

// ============================================================
// GROUND PRESSURE CALIBRATION
// ============================================================

bool BMP180::calibrateGroundPressure(uint8_t samples) {
    if (samples < 3) {
        Serial.println(
            "[BMP180] At least 3 samples are required."
        );

        return false;
    }

    Serial.println("\n[BMP180] Ground calibration");

    Serial.println(
        "[BMP180] Keep the rocket stationary at the launch site."
    );

    Serial.printf(
        "[BMP180] Collecting %u pressure samples...\n",
        static_cast<unsigned>(samples)
    );

    // uint8_t samples has a maximum value of 255.
    float pressureSamples[255];

    uint16_t validSamples = 0;

    for (uint16_t i = 0; i < samples; i++) {
        uint16_t ut = 0;
        uint32_t up = 0;

        int32_t b5 = 0;

        float temperature = 0.0f;
        uint32_t pressure = 0;

        // Read temperature
        if (!readRawTemperature(ut)) {
            Serial.printf(
                "[BMP180] Temperature read failed at sample %u.\n",
                static_cast<unsigned>(i + 1)
            );

            continue;
        }

        if (!calculateTemperature(
                ut,
                b5,
                temperature
            )) {
            continue;
        }

        // Read pressure
        if (!readRawPressure(up)) {
            Serial.printf(
                "[BMP180] Pressure read failed at sample %u.\n",
                static_cast<unsigned>(i + 1)
            );

            continue;
        }

        if (!calculatePressure(
                up,
                b5,
                pressure
            )) {
            continue;
        }

        // Basic physical plausibility check:
        // 30 kPa to 120 kPa.
        if (pressure < 30000UL || pressure > 120000UL) {
            Serial.printf(
                "[BMP180] Invalid pressure sample: %lu Pa\n",
                static_cast<unsigned long>(pressure)
            );

            continue;
        }

        pressureSamples[validSamples] =
            static_cast<float>(pressure);

        validSamples++;

        Serial.printf(
            "[BMP180] Sample %u/%u: %.2f hPa\n",
            static_cast<unsigned>(i + 1),
            static_cast<unsigned>(samples),
            pressure / 100.0f
        );
    }

    // Require at least 80% valid samples
    uint16_t minimumValidSamples =
        (static_cast<uint16_t>(samples) * 8 + 9) / 10;

    if (minimumValidSamples < 3) {
        minimumValidSamples = 3;
    }

    if (validSamples < minimumValidSamples) {
        Serial.println(
            "[BMP180] Not enough valid pressure samples."
        );

        return false;
    }

    // --------------------------------------------------------
    // SORT PRESSURE SAMPLES (INSERTION SORT)
    // --------------------------------------------------------

    for (uint16_t i = 1; i < validSamples; i++) {
        float key = pressureSamples[i];
        int32_t j = static_cast<int32_t>(i) - 1;

        while (
            j >= 0 &&
            pressureSamples[j] > key
        ) {
            pressureSamples[j + 1] = pressureSamples[j];
            j--;
        }

        pressureSamples[j + 1] = key;
    }

    // --------------------------------------------------------
    // CALCULATE MEDIAN PRESSURE
    // --------------------------------------------------------

    if (validSamples % 2 == 0) {
        uint16_t middle = validSamples / 2;

        groundPressure = (
            pressureSamples[middle - 1] +
            pressureSamples[middle]
        ) / 2.0f;
    } else {
        uint16_t middle = validSamples / 2;

        groundPressure = pressureSamples[middle];
    }

    if (groundPressure <= 0.0f) {
        Serial.println(
            "[BMP180] Ground pressure calibration failed."
        );

        return false;
    }

    // Reset baseline and filter after calibration
    resetAltitudeFilter();

    Serial.println(
        "\n[BMP180] Ground calibration complete."
    );

    Serial.printf(
        "[BMP180] Valid samples: %u/%u\n",
        static_cast<unsigned>(validSamples),
        static_cast<unsigned>(samples)
    );

    Serial.printf(
        "[BMP180] Reference pressure: %.2f hPa\n",
        groundPressure / 100.0f
    );

    Serial.println(
        "[BMP180] Relative altitude will start at 0.00 m on the first reading."
    );

    return true;
}

// ============================================================
// GET REFERENCE PRESSURE
// ============================================================

float BMP180::getGroundPressure() const {
    return groundPressure;
}

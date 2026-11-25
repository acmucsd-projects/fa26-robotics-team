#pragma once

// Wrapper for the BMI160 IMU (accelerometer + gyroscope) over I2C.
// Usage: Imu imu; imu.begin(); imu.calibrateGyro(); imu.read(r);

#include "DFRobot_BMI160.h"
#include "Vector.hpp"
#include <Arduino.h>

// I2C pins and address of the BMI160
constexpr uint8_t IMU_SDA = 6;
constexpr uint8_t IMU_SCL = 7;
constexpr uint8_t IMU_ADDR = 0x68;

// Raw readings are 16-bit counts (-32768..32767). These turn counts into units.
// Accel range is +-4g, so 32768 / 4 = 8192 counts per g.
// Gyro range is +-2000 deg/s, so 32768 / 2000 = 16.4 counts per deg/s.
constexpr float ACCEL_LSB_PER_G = 8192.0f;
constexpr float GYRO_LSB_PER_DPS = 16.4f;

// accel x y z in m/s^2, gyro x y z in deg/s.
struct ImuReading {
  Vec3 accel;
  Vec3 gyro;
};

class Imu {
public:
  // accelOffset (raw counts) and accelScale come from the calibrate firmware in
  // CalibrateIMU.cpp. (pio run -e calibrate -t upload -t monitor), which prints
  // a ready-made line.
  Imu(Vec3 accelOffset = 0, Vec3 accelScale = 1);

  // Starts I2C and sets accel to +-4g @ 100 Hz, gyro to +-2000 deg/s @ 100 Hz.
  bool begin();
  // Averages the gyro and saves it as the zero point.
  void calibrateGyro(int samples = 200);
  // Averages raw counts over many samples. Used by calibrateGyro and
  // CalibrateIMU.
  void averageRaw(Vec3 &gyro, Vec3 &accel, int samples);
  bool read(ImuReading &out);

private:
  bool writeReg(uint8_t reg, uint8_t value);
  bool readRaw(int16_t *d);

  DFRobot_BMI160 bmi160;
  // All three are in raw counts, not real units.
  Vec3 gyroBias = 0;
  Vec3 accelOffset;
  Vec3 accelScale;
};

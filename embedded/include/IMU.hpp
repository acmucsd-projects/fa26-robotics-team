// Wrapper for the BMI160 IMU (accelerometer + gyroscope) over I2C.
// Usage: Imu imu; imu.begin(); imu.calibrateGyro(); imu.read(r);

#pragma once
#include "Matrix.hpp"
#include "Vector.hpp"
#include <Arduino.h>
#include <DFRobot_BMI160.h>
#include <cstdint>

// I2C pins and address of the BMI160
constexpr uint8_t IMU_SDA = 6;
constexpr uint8_t IMU_SCL = 7;
constexpr uint8_t IMU_ADDR = 0x68;

// Raw readings are 16-bit ticks (-32768..32767). These turn ticks into units.
// Accel range is +-4g, so 32768 / 4 = 8192 ticks per g.
// Gyro range is +-2000 deg/s, so 32768 / 2000 = 16.4 ticks per deg/s.
constexpr float ACCEL_LSB_PER_G = 8192.0F;
constexpr float GYRO_LSB_PER_DPS = 16.4F;

class Imu {
  static bool writeReg(uint8_t reg, uint8_t value);

  DFRobot_BMI160 bmi160_;

  // All three are in raw ticks.
  const Vec3<> gyroBias_;
  const Vec3<> accelBias_;
  const Vec3<> accelScale_;
  const Mat3<> orientation_;

public:
  // Gyro bias, acceleration bias, and acceleration scale in ticks from CalibrateIMU.cpp.
  // pio run -e calibrate -t upload -t monitor prints a ready-made line.
  Imu(Vec3<> gyroBias = 0.0F, Vec3<> accelBias = 0.0F, Vec3<> accelScale = 1.0F,
      Mat3<> orientation = Mat3<>::identity());

  // Starts I2C and sets accel to +-4g @ 100 Hz, gyro to +-2000 deg/s @ 100 Hz.
  bool begin();

  // Read raw ticks.
  bool readRaw(Vec3<> &rawGyro, Vec3<> &rawAccel);

  // Averages raw ticks over many samples.
  bool avgRaw(Vec3<> &avgRawGyro, Vec3<> &avgRawAccel, size_t samples = 1000);

  // Read calibrated, unit-converted data.
  // accel x, y, z in m/s^2.
  // gyro x (roll), y (pitch), z (yaw) in deg/s.
  bool read(Vec3<> &gyro, Vec3<> &accel);
};

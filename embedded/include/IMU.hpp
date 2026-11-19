// Wrapper for the BMI160 IMU (accelerometer + gyroscope) over I2C.
// Usage: Imu imu; imu.begin(); imu.calibrateGyro(); imu.read(r);

#pragma once
#include "Vector.hpp"
#include <Arduino.h>
#include <DFRobot_BMI160.h>

// I2C pins and address of the BMI160
constexpr uint8_t IMU_SDA = 6;
constexpr uint8_t IMU_SCL = 7;
constexpr uint8_t IMU_ADDR = 0x68;

// Raw readings are 16-bit counts (-32768..32767). These turn counts into units.
// Accel range is +-4g, so 32768 / 4 = 8192 counts per g.
// Gyro range is +-2000 deg/s, so 32768 / 2000 = 16.4 counts per deg/s.
constexpr float ACCEL_LSB_PER_G = 8192.0F;
constexpr float GYRO_LSB_PER_DPS = 16.4F;

// accel x y z in m/s^2, gyro x y z in deg/s.
struct ImuReading {
  Vec3<> accel;
  Vec3<> gyro;
};

class Imu {
public:
  // accelBias (raw counts) and accelScale come from the calibrate firmware in
  // CalibrateIMU.cpp. (pio run -e calibrate -t upload -t monitor), which prints
  // a ready-made line.
  Imu(Vec3<> gyroBias = 0.0F, Vec3<> accelBias = 0.0F, Vec3<> accelScale = 1.0F);

  // Starts I2C and sets accel to +-4g @ 100 Hz, gyro to +-2000 deg/s @ 100 Hz.
  bool begin();

  // Averages raw counts over many samples. Used by calibrateGyro and
  // CalibrateIMU.
  void avgRaw(Vec3<> &gyro, Vec3<> &accel, int samples = 1000);
  bool read(ImuReading &out);

private:
  static bool writeReg(uint8_t reg, uint8_t value);
  static bool readRaw(int16_t *d);

  DFRobot_BMI160 bmi160_;

  // All three are in raw ticks, not real units.
  Vec3<> gyroBias_;
  Vec3<> accelBias_;
  Vec3<> accelScale_;
};

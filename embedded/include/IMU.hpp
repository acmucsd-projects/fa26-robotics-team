#pragma once

/* wrapper for the BMI160 IMU (accel + gyro) over I2C */
/* usage: Imu imu(sda, scl); imu.begin(); imu.calibrateGyro(); imu.read(r); */

#include "DFRobot_BMI160.h" /* reverts to DFRobot */
#include "Vector.hpp"       /* Vec3 from the IMU branch, holds x y z together */
#include <Arduino.h>

/* a 16bit raw values ranges -32768 to +32767 */
/* IMU is set to +-4g, so 32768 / 4 = 8192 */
/* this is so that raw value / 8192 equals to 1g */
constexpr float ACCEL_LSB_PER_G = 8192.0f;

/* accel = x y z in m/s^2, gyro = x y z in deg/s */
/* switched from 6 floats to Vec3 so main.cpp can use r.accel.x */
struct ImuReading {
  Vec3<float> accel;
  Vec3<float> gyro;
};

class Imu {
public:
  /* accelOffset (raw counts) and accelScale come from the calibrate firmware */
  /* (pio run -e calibrate), it prints a line to paste into main.cpp */
  Imu(uint8_t sda, uint8_t scl, int8_t addr = 0x68, Vec3<float> accelOffset = 0,
      Vec3<float> accelScale = 1);

  /* starts I2C on our pins, sets accel to +-4g 100hz and gyro to 100hz */
  bool begin();
  /* board has to sit still, averages the gyro and saves it as the bias */
  void calibrateGyro(int samples = 200);
  /* used by calibrateGyro and the calibrate firmware */
  void averageRaw(Vec3<float> &gyro, Vec3<float> &accel, int samples);
  bool read(ImuReading &out);

private:
  /* the library has no setter for range and rate, so we write registers
   * ourselves */
  bool writeReg(uint8_t reg, uint8_t value);
  bool readRaw(int16_t *d);

  DFRobot_BMI160 bmi160;
  uint8_t sda, scl;
  int8_t addr;
  /* all three are in raw counts, not real units */
  Vec3<float> gyroBias = 0;
  Vec3<float> accelOffset;
  Vec3<float> accelScale;
};

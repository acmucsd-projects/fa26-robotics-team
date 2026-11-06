#pragma once

#include "DFRobot_BMI160.h"
#include <Arduino.h>

/* this makes the readings readable*/
constexpr float ACCEL_LSB_PER_G = 8192.0f;
constexpr float ACCEL_MS2_PER_LSB = 9.80665f / ACCEL_LSB_PER_G;

struct ImuReading {
  float ax, ay, az;
  float gx, gy, gz;
};

class Imu {
public:
  Imu(uint8_t sda, uint8_t scl, int8_t addr = BMI160_I2C_ADDR);

  bool begin();
  void calibrateGyro(int samples = 200);
  void averageRaw(float out[6], int samples);
  void saveAccelCalibration(float offset[3], float scale[3]);
  bool read(ImuReading &out);

private:
  bool writeReg(uint8_t reg, uint8_t value);

  DFRobot_BMI160 bmi160;
  uint8_t sda, scl;
  int8_t addr;
  float gyroBias[3] = {};
  float accelOffset[3] = {};
  float accelScale[3] = {1, 1, 1};
};

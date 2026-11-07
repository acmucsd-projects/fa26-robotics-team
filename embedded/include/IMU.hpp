#pragma once

#include "DFRobot_BMI160.h"
#include <Arduino.h>

/* unit conversion from 16-bit to m/s^2 */
/* imu.cpp sets accel to +-4g, and the raw value is 16bits, */
/* so it runs from -32768 to +32768, dividing 32768 / 4g gives 8192 as 1g */
constexpr float ACCEL_LSB_PER_G = 8192.0f;
constexpr float ACCEL_MS2_PER_LSB = 9.80665f / ACCEL_LSB_PER_G;

/* ax, ay, az is acceleration in m/s^2 */
/* gx, gy, gz is rotation speed in deg/s  */
struct ImuReading {
  float ax, ay, az;
  float gx, gy, gz;
};

class Imu {
public:
  Imu(uint8_t sda, uint8_t scl, int8_t addr = BMI160_I2C_ADDR);

  bool begin();
  void calibrateGyro(
      int samples = 200); /* takes 200 readings and saves avg value as bias */
  void averageRaw(float out[6], int samples);
  void
  saveAccelCalibration(float offset[3],
                       float scale[3]); /* saves accel calibration to flash */
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

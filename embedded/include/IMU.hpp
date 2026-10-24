#pragma once

#include "DFRobot_BMI160.h"
#include <Arduino.h>

struct ImuReading {
  float ax, ay, az;
  float gx, gy, gz;
};

class Imu {
public:
  Imu(uint8_t sda, uint8_t scl, int8_t addr = BMI160_I2C_ADDR);

  bool begin();
  void calibrateGyro(int samples = 200);
  bool read(ImuReading &out);

private:
  bool writeReg(uint8_t reg, uint8_t value);

  DFRobot_BMI160 bmi160;
  uint8_t sda, scl;
  int8_t addr;
  float gyroBias[3] = {};
};

#include "IMU.hpp"
#include "BMI160.h"
#include "Vector.hpp"
#include <BMI160Gen.h>
#include <Wire.h>`
#include <cstdint>

IMU::IMU(uint8_t sda_, uint8_t scl_, Vec3<float> accelBias_, Vec3<float> accelScale_,
         Vec3<float> gyroBias_)
    : sda{sda_}, scl{scl_}, accelBias{accelBias_}, accelScale{accelScale_}, gyroBias{gyroBias_},
      accelUnitConv{9.80665F / 8192.0F}, gyroUnitConv{} {}

bool IMU::begin() const {
  Serial.begin(115200);
  while (!Serial) {
  }
  Wire.begin(sda, scl);
  Wire.setClock(100000);
  return BMI160.begin(BMI160GenClass::I2C_MODE, 0x68);
}

bool IMU::readTicks(IMUReading &out) {
  Vec3<int> rawAccel{};
  Vec3<int> rawGyro{};
  BMI160.readMotionSensor(rawAccel.x, rawAccel.y, rawAccel.z, rawGyro.x, rawGyro.y, rawGyro.z);

  out.accel = (rawAccel - accelBias) * accelScale;
  out.gyro = rawGyro - gyroBias;
  return true;
}

bool IMU::read(IMUReading &out) {
  bool res{readTicks(out)};
  out.accel *= accelUnitConv;
  out.gyro *= gyroUnitConv;
  return res;
}

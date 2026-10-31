#include "IMU.hpp"
#include "Vector.hpp"
#include <Wire.h>
#include <cstdint>

IMU::IMU(uint8_t sda_, uint8_t scl_, Vec3<float> accelBias_, Vec3<float> accelScale_,
         Vec3<float> gyroBias_)
    : sda{sda_}, scl{scl_}, accelBias{accelBias_}, accelScale{accelScale_}, gyroBias{gyroBias_},
      accelUnitConv{9.80665F / 8192.0F}, gyroUnitConv{} {}

bool IMU::begin() const {
  Wire.begin(sda, scl);
  return true;
}

bool IMU::readTicks(IMUReading &out) {
  std::array<int16_t, 6> data{};

  Vec3<float> rawAccel{
      static_cast<float>(data[0]),
      static_cast<float>(data[1]),
      static_cast<float>(data[2]),
  };
  Vec3<float> rawGyro{
      static_cast<float>(data[3]),
      static_cast<float>(data[4]),
      static_cast<float>(data[5]),
  };

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

bool IMU::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(BMI160_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

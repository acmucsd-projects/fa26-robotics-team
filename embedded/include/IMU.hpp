#pragma once

#include "Vector.hpp"
#include <BMI160Gen.h>
#include <cstdint>

struct IMUReading {
  Vec3<float> accel;
  Vec3<float> gyro;
  IMUReading operator+(IMUReading other);
};

class IMU {
  const uint8_t sda;
  const uint8_t scl;

  const Vec3<float> accelBias;
  const Vec3<float> accelScale;
  const Vec3<float> gyroBias;

  const Vec3<float> accelUnitConv;
  const Vec3<float> gyroUnitConv;

public:
  IMU(uint8_t sda_, uint8_t scl_, Vec3<float> accelBias_ = 0,
      Vec3<float> accelScale_ = 1, Vec3<float> gyroBias_ = 0);
  [[nodiscard]] bool begin() const;
  [[nodiscard]] bool readTicks(IMUReading &out);
  [[nodiscard]] bool read(IMUReading &out);
};

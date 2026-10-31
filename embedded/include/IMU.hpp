#pragma once

#include "Vector.hpp"
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
  IMU(uint8_t sda, uint8_t scl, Vec3<float> accelBias = 0, Vec3<float> accelScale = 1,
      Vec3<float> gyroBias = 0);
  [[nodiscard]] bool begin() const;
  bool readTicks(IMUReading &out);
  bool read(IMUReading &out);
  [[nodiscard]] static bool writeReg(uint8_t reg, uint8_t value);
};

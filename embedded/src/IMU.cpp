#include "IMU.hpp"
#include "Array.hpp"
#include "Matrix.hpp"
#include "Vector.hpp"
#include <Wire.h>
#include <cstdint>

Imu::Imu(Vec3<> gyroBias, Vec3<> accelBias, Vec3<> accelScale, Mat3<> orientation)
    : gyroBias_(gyroBias), accelBias_(accelBias), accelScale_(accelScale),
      orientation_(orientation) {}

bool Imu::begin() {
  Wire.end();
  Wire.begin(IMU_SDA, IMU_SCL);

  // The library has no setters for range and rate, so we write the registers.
  // Config byte = filter mode in the top 4 bits | output rate in the bottom 4.
  // To change a range, also change the matching constant in IMU.hpp.
  return bmi160_.I2cInit(IMU_ADDR) == BMI160_OK &&
         writeReg(BMI160_ACCEL_CONFIG_ADDR,
                  BMI160_ACCEL_BW_OSR4_AVG1 << 4 | BMI160_ACCEL_ODR_100HZ) &&
         writeReg(BMI160_ACCEL_RANGE_ADDR, BMI160_ACCEL_RANGE_4G) &&
         writeReg(BMI160_GYRO_CONFIG_ADDR, BMI160_GYRO_BW_OSR4_MODE << 4 | BMI160_GYRO_ODR_100HZ) &&
         writeReg(BMI160_GYRO_RANGE_ADDR, BMI160_GYRO_RANGE_2000_DPS);
}

bool Imu::avgRaw(Vec3<> &avgRawGyro, Vec3<> &avgRawAccel, size_t samples) {
  Vec3<> sumGyro;
  Vec3<> sumAccel;

  // n only goes up on a good read, so we always average exactly `samples`.
  for (size_t n{}; n < samples; ++n) {
    Vec3<> gyro;
    Vec3<> accel;
    if (!readRaw(gyro, accel)) {
      return false;
    }
    sumGyro += gyro;
    sumAccel += accel;
  }
  avgRawGyro = sumGyro / static_cast<float>(samples);
  avgRawAccel = sumAccel / static_cast<float>(samples);
  return true;
}

// Reads all 6 values in one go: 12 bytes starting at the gyro data registe
// Each value is 2 bytes, low byte first. Result: d = gyro x y z, accel x y z.
bool Imu::readRaw(Vec3<> &rawGyro, Vec3<> &rawAccel) {
  Array<int16_t, 6> raw;
  if (bmi160_.getAccelGyroData(raw.data()) != 0) {
    return false;
  }
  rawGyro.x = raw[0];
  rawGyro.y = raw[1];
  rawGyro.z = raw[2];
  rawAccel.x = raw[3];
  rawAccel.y = raw[4];
  rawAccel.z = raw[5];
  return true;
}

bool Imu::read(Vec3<> &gyro, Vec3<> &accel) {
  Vec3<> rawGyro;
  Vec3<> rawAccel;
  if (!readRaw(rawGyro, rawAccel)) {
    return false;
  }

  // Accel: remove offset, fix sensitivity, then ticks -> m/s^2.
  accel = (rawAccel - accelBias_) * accelScale_ * (9.80665F / ACCEL_LSB_PER_G);

  // Gyro: remove bias, then ticks -> deg/s.
  gyro = (rawGyro - gyroBias_) / GYRO_LSB_PER_DPS;

  return true;
}

// Sends 2 bytes: which register, then the value. True if the chip answered.
bool Imu::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

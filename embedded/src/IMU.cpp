#include "IMU.hpp"
#include <Wire.h>

constexpr float ACCEL_MS2_PER_LSB = 9.80665f / 8192.0f;
constexpr float GYRO_DPS_PER_LSB = 1.0f / 16.4f;

Imu::Imu(uint8_t sda, uint8_t scl, int8_t addr)
    : sda(sda), scl(scl), addr(addr) {}

bool Imu::begin() {
  Wire.begin(sda, scl);

  bool ok = bmi160.I2cInit(addr) == BMI160_OK &&
            writeReg(BMI160_ACCEL_CONFIG_ADDR,
                     BMI160_ACCEL_BW_OSR4_AVG1 << 4 | BMI160_ACCEL_ODR_100HZ) &&
            writeReg(BMI160_ACCEL_RANGE_ADDR, BMI160_ACCEL_RANGE_4G) &&
            writeReg(BMI160_GYRO_CONFIG_ADDR,
                     BMI160_GYRO_BW_OSR4_MODE << 4 | BMI160_GYRO_ODR_100HZ);
  delay(100);
  return ok;
}

void Imu::calibrateGyro(int samples) {
  long sum[3] = {};
  int16_t d[6];
  for (int n = 0; n < samples;) {
    if (bmi160.getAccelGyroData(d) == BMI160_OK) {
      for (int i = 0; i < 3; i++)
        sum[i] += d[i];
      n++;
    }
    delay(10);
  }
  for (int i = 0; i < 3; i++)
    gyroBias[i] = sum[i] / float(samples);
}

bool Imu::read(ImuReading &out) {
  int16_t d[6];
  if (bmi160.getAccelGyroData(d) != BMI160_OK)
    return false;

  out.ax = d[3] * ACCEL_MS2_PER_LSB;
  out.ay = d[4] * ACCEL_MS2_PER_LSB;
  out.az = d[5] * ACCEL_MS2_PER_LSB;
  out.gx = (d[0] - gyroBias[0]) * GYRO_DPS_PER_LSB;
  out.gy = (d[1] - gyroBias[1]) * GYRO_DPS_PER_LSB;
  out.gz = (d[2] - gyroBias[2]) * GYRO_DPS_PER_LSB;
  return true;
}

bool Imu::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

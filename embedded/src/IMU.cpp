#include "IMU.hpp"
#include <Wire.h>

/* gyro range is +-2000 dps (dfrobot default), so 32768 / 2000 = 16.4 raw = 1
 * deg/s */
constexpr float GYRO_DPS_PER_LSB = 1.0f / 16.4f;

/* calibration now comes in here instead of being loaded from flash */
Imu::Imu(uint8_t sda, uint8_t scl, int8_t addr, Vec3<float> accelOffset,
         Vec3<float> accelScale)
    : sda(sda), scl(scl), addr(addr), accelOffset(accelOffset),
      accelScale(accelScale) {}

bool Imu::begin() {
  Wire.end();
  Wire.begin(sda, scl);

  /* ok is only true if every step works */
  /* config byte = filter mode in the top half (<< 4) | output rate in the
   * bottom half */
  /* accel range gets bumped from the library default +-2g to +-4g, cus why not
   */
  bool ok = bmi160.I2cInit(addr) == BMI160_OK &&
            writeReg(BMI160_ACCEL_CONFIG_ADDR,
                     BMI160_ACCEL_BW_OSR4_AVG1 << 4 | BMI160_ACCEL_ODR_100HZ) &&
            writeReg(BMI160_ACCEL_RANGE_ADDR, BMI160_ACCEL_RANGE_4G) &&
            writeReg(BMI160_GYRO_CONFIG_ADDR,
                     BMI160_GYRO_BW_OSR4_MODE << 4 | BMI160_GYRO_ODR_100HZ);
  /* give the chip time to settle */
  delay(100);
  return ok;
}

void Imu::averageRaw(float out[6], int samples) {
  /* long because hundreds of 16bit readings added up overflow an int16 */
  long sum[6] = {};
  int16_t d[6];
  /* n only goes up on a good read, so we always average exactly samples
   * readings */
  for (int n = 0; n < samples;) {
    if (bmi160.getAccelGyroData(d) == BMI160_OK) {
      for (int i = 0; i < 6; i++)
        sum[i] += d[i];
      n++;
    }
    /* 10ms = one new reading at 100hz */
    delay(10);
  }
  for (int i = 0; i < 6; i++)
    out[i] = sum[i] / float(samples);
}

void Imu::calibrateGyro(int samples) {
  float avg[6];
  averageRaw(avg, samples);
  /* gyro is the first 3, a still gyro should read 0 so whatever it reads is
   * bias */
  gyroBias = Vec3<float>(avg[0], avg[1], avg[2]);
}

bool Imu::read(ImuReading &out) {
  int16_t d[6];
  if (bmi160.getAccelGyroData(d) != BMI160_OK)
    return false;

  /* d is gyro 0-2 then accel 3-5 */
  Vec3<float> rawAccel(d[3], d[4], d[5]);
  Vec3<float> rawGyro(d[0], d[1], d[2]);
  /* accel: remove offset, fix sensitivity with scale, then raw to m/s^2 */
  out.accel = (rawAccel - accelOffset) * accelScale * ACCEL_MS2_PER_LSB;
  /* gyro: remove bias, then raw to deg/s */
  out.gyro = (rawGyro - gyroBias) * GYRO_DPS_PER_LSB;
  return true;
}

/* sends 2 bytes to the chip: which register, then the value to put in it */
/* returns true if the chip acknowledged */
bool Imu::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

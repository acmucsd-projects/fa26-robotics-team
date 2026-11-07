#include "IMU.hpp"
#include <Wire.h>

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
  /* gyro range is left at the default +-2000 dps. to change it, uncomment the
   * line below, swap 2000 for 1000, 500, 250 or 125, and change 16.4f in
   * read() to 32.8f, 65.6f, 131.2f or 262.4f to match */
  /* ok = ok && writeReg(BMI160_GYRO_RANGE_ADDR, BMI160_GYRO_RANGE_2000_DPS); */
  /* give the chip time to settle */
  delay(100);
  return ok;
}

void Imu::averageRaw(Vec3<float> &gyro, Vec3<float> &accel, int samples) {
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
  gyro = Vec3<float>(sum[0], sum[1], sum[2]) / float(samples);
  accel = Vec3<float>(sum[3], sum[4], sum[5]) / float(samples);
}

void Imu::calibrateGyro(int samples) {
  Vec3<float> accel;
  averageRaw(gyroBias, accel, samples);
}

bool Imu::read(ImuReading &out) {
  int16_t d[6];
  if (bmi160.getAccelGyroData(d) != BMI160_OK)
    return false;

  /* d is gyro 0-2 then accel 3-5 */
  Vec3<float> rawAccel(d[3], d[4], d[5]);
  Vec3<float> rawGyro(d[0], d[1], d[2]);
  /* accel: remove offset, fix sensitivity with scale, then raw to m/s^2 */
  out.accel = (rawAccel - accelOffset) * accelScale * (9.80665f / ACCEL_LSB_PER_G);
  /* gyro: remove bias, then raw to deg/s */
  out.gyro = (rawGyro - gyroBias) / 16.4f;
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

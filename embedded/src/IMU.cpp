#include "IMU.hpp"
#include <Preferences.h>
#include <Wire.h>

/* gyro version of the accel */
/* gyro range is +-2000 dps on a 16bit raw val */
/* so 32768 / 2000 = 16.4 counts = 1 deg/s */
/* begin() never sets the gyro range, +-2000 is the dfrobot library default */
/* change the range and this number has to change too */
/* note: 2000 is the max spin it can measure, not how fast it moves */
/* gimbal is way slower, +-500 (65.6) would give finer steps if tilt gets
 * jittery */
constexpr float GYRO_DPS_PER_LSB = 1.0f / 16.4f;

/* saves pins and addresses */
Imu::Imu(uint8_t sda, uint8_t scl, int8_t addr)
    : sda(sda), scl(scl), addr(addr) {}

/* starts i2c, sets up the chip, loads accel calibration from flash */
bool Imu::begin() {
  Wire.begin(sda, scl);

  /* ok is only true if all 4 steps work, if one fails the rest get skipped */
  /* config byte = filter mode in the top half (<< 4) | output rate in the
   * bottom half */
  /* accel range gets bumped from the library default +-2g to +-4g */
  bool ok = bmi160.I2cInit(addr) == BMI160_OK &&
            writeReg(BMI160_ACCEL_CONFIG_ADDR,
                     BMI160_ACCEL_BW_OSR4_AVG1 << 4 | BMI160_ACCEL_ODR_100HZ) &&
            writeReg(BMI160_ACCEL_RANGE_ADDR, BMI160_ACCEL_RANGE_4G) &&
            writeReg(BMI160_GYRO_CONFIG_ADDR,
                     BMI160_GYRO_BW_OSR4_MODE << 4 | BMI160_GYRO_ODR_100HZ);
  delay(100);

  /* load saved accel calibration, if nothing was saved it stays offset 0 scale
   * 1 */
  /* "imu", "offset", "scale" have to match saveAccelCalibration() */
  Preferences prefs;
  prefs.begin("imu", false);
  prefs.getBytes("offset", accelOffset, sizeof(accelOffset));
  prefs.getBytes("scale", accelScale, sizeof(accelScale));
  prefs.end();
  return ok;
}

/* averages raw readings, out order is gyro x y z then accel x y z */
void Imu::averageRaw(float out[6], int samples) {
  /* long because 200 raw readings added up can overflow int16 */
  long sum[6] = {};
  int16_t d[6];
  for (int n = 0; n < samples;) {
    if (bmi160.getAccelGyroData(d) == BMI160_OK) {
      for (int i = 0; i < 6; i++)
        sum[i] += d[i];
      n++;
    }
    delay(10);
  }
  /* float so the division keeps the decimals */
  for (int i = 0; i < 6; i++)
    out[i] = sum[i] / float(samples);
}

/* board has to sit still, averages the gyro and saves it as the bias (raw
 * counts) */
void Imu::calibrateGyro(int samples) {
  float avg[6];
  averageRaw(avg, samples);
  for (int i = 0; i < 3; i++)
    gyroBias[i] = avg[i];
}

void Imu::saveAccelCalibration(float offset[3], float scale[3]) {
  Preferences prefs;
  prefs.begin("imu", false);
  prefs.putBytes("offset", offset, 3 * sizeof(float));
  prefs.putBytes("scale", scale, 3 * sizeof(float));
  prefs.end();
  for (int i = 0; i < 3; i++) {
    accelOffset[i] = offset[i];
    accelScale[i] = scale[i];
  }
}

bool Imu::read(ImuReading &out) {
  int16_t d[6];
  if (bmi160.getAccelGyroData(d) != BMI160_OK)
    return false;

  /* d is gyro 0-2 then accel 3-5, so ax comes from d[3] */
  /* accel: remove offset, fix sensitivity with scale, then counts to m/s^2 */
  out.ax = (d[3] - accelOffset[0]) * accelScale[0] * ACCEL_MS2_PER_LSB;
  out.ay = (d[4] - accelOffset[1]) * accelScale[1] * ACCEL_MS2_PER_LSB;
  out.az = (d[5] - accelOffset[2]) * accelScale[2] * ACCEL_MS2_PER_LSB;
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

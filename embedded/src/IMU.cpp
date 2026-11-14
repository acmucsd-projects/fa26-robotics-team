#include "IMU.hpp"
#include <Wire.h>

Imu::Imu(uint8_t sda, uint8_t scl, int8_t addr, Vec3<float> accelOffset,
         Vec3<float> accelScale)
    : sda(sda), scl(scl), addr(addr), accelOffset(accelOffset),
      accelScale(accelScale) {}

bool Imu::begin() {
  // The DFRobot library starts Wire on the default pins, so restart it on ours.
  Wire.end();
  Wire.begin(sda, scl);

  // The library has no setters for range and rate, so we write the registers.
  // Config byte = filter mode in the top 4 bits | output rate in the bottom 4.
  // To change a range, also change the matching constant in IMU.hpp.
  bool ok = bmi160.I2cInit(addr) == BMI160_OK &&
            writeReg(BMI160_ACCEL_CONFIG_ADDR,
                     BMI160_ACCEL_BW_OSR4_AVG1 << 4 | BMI160_ACCEL_ODR_100HZ) &&
            writeReg(BMI160_ACCEL_RANGE_ADDR, BMI160_ACCEL_RANGE_4G) &&
            writeReg(BMI160_GYRO_CONFIG_ADDR,
                     BMI160_GYRO_BW_OSR4_MODE << 4 | BMI160_GYRO_ODR_100HZ) &&
            writeReg(BMI160_GYRO_RANGE_ADDR, BMI160_GYRO_RANGE_2000_DPS);
  // Give the chip time to settle.
  delay(100);
  return ok;
}

void Imu::averageRaw(Vec3<float> &gyro, Vec3<float> &accel, int samples) {
  // long, because hundreds of 16-bit readings added up overflow an int16.
  long sum[6] = {};
  int16_t d[6];
  // n only goes up on a good read, so we always average exactly `samples`.
  for (int n = 0; n < samples;) {
    if (readRaw(d)) {
      for (int i = 0; i < 6; i++)
        sum[i] += d[i];
      n++;
    }
    // 10 ms = one new reading at 100 Hz.
    delay(10);
  }
  gyro = Vec3<float>(sum[0], sum[1], sum[2]) / float(samples);
  accel = Vec3<float>(sum[3], sum[4], sum[5]) / float(samples);
}

void Imu::calibrateGyro(int samples) {
  Vec3<float> accel;
  averageRaw(gyroBias, accel, samples);
}

// Reads all 6 values in one go: 12 bytes starting at the gyro data register.
// Each value is 2 bytes, low byte first. Result: d = gyro x y z, accel x y z.
bool Imu::readRaw(int16_t *d) {
  Wire.beginTransmission(addr);
  Wire.write(BMI160_GYRO_DATA_ADDR);
  if (Wire.endTransmission() != 0)
    return false;
  if (Wire.requestFrom(addr, 12) != 12)
    return false;
  for (int i = 0; i < 6; i++) {
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    d[i] = (int16_t)(msb << 8 | lsb);
  }
  return true;
}

bool Imu::read(ImuReading &out) {
  int16_t d[6];
  if (!readRaw(d))
    return false;

  Vec3<float> rawGyro(d[0], d[1], d[2]);
  Vec3<float> rawAccel(d[3], d[4], d[5]);
  // Accel: remove offset, fix sensitivity, then counts -> m/s^2.
  out.accel =
      (rawAccel - accelOffset) * accelScale * (9.80665f / ACCEL_LSB_PER_G);
  // Gyro: remove bias, then counts -> deg/s.
  out.gyro = (rawGyro - gyroBias) / GYRO_LSB_PER_DPS;
  return true;
}

// Sends 2 bytes: which register, then the value. True if the chip answered.
bool Imu::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// Points the servos at the board's tilt, from accel only (no filter).
void MovePos(int id, int position_number);
bool imuDriveMotors(Imu &imu) {
  ImuReading r;
  if (!imu.read(r))
    return false;

  float pitch =
      atan2(-r.accel.x, sqrt(r.accel.y * r.accel.y + r.accel.z * r.accel.z)) *
      180.0 / PI;
  float roll = atan2(r.accel.y, r.accel.z) * 180.0 / PI;

  // 4096 servo steps per turn, 2048 = center, clamped to +-90 degrees.
  MovePos(1, constrain(2048 + pitch * 4096.0 / 360.0, 1024, 3072));
  MovePos(2, constrain(2048 + roll * 4096.0 / 360.0, 1024, 3072));
  return true;
}

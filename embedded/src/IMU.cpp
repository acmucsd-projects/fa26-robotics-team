#include "IMU.hpp"
#include "Array.hpp"
#include "Vector.hpp"
#include <Wire.h>

Imu::Imu(Vec3<> gyroBias, Vec3<> accelBias, Vec3<> accelScale)
    : gyroBias_(gyroBias), accelBias_(accelBias), accelScale_(accelScale) {}

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

void Imu::avgRaw(Vec3<> &gyro, Vec3<> &accel, int samples) {
  // long, because hundreds of 16-bit readings added up overflow an int16.
  Array<float, 6> sum;
  Array<int16_t, 6> d;
  // n only goes up on a good read, so we always average exactly `samples`.
  for (int n = 0; n < samples; ++n) {
    if (readRaw(d.data())) {
      for (int i = 0; i < 6; i++) {
        sum[i] += static_cast<float>(d[i]);
      }
    }
    // 10 ms = one new reading at 100 Hz.
    delay(10);
  }
  gyro = Vec3<>{sum[0], sum[1], sum[2]} / static_cast<float>(samples);
  accel = Vec3<>{sum[3], sum[4], sum[5]} / static_cast<float>(samples);
}

// Reads all 6 values in one go: 12 bytes starting at the gyro data register.
// Each value is 2 bytes, low byte first. Result: d = gyro x y z, accel x y z.
bool Imu::readRaw(int16_t *d) {
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(BMI160_GYRO_DATA_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  if (Wire.requestFrom(IMU_ADDR, 12) != 12) {
    return false;
  }
  for (int i = 0; i < 6; i++) {
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    d[i] = static_cast<int16_t>(msb << 8 | lsb);
  }
  return true;
}

bool Imu::read(ImuReading &out) {
  Array<int16_t, 6> d;
  if (!readRaw(d.data())) {
    return false;
  }

  Vec3<> rawGyro{static_cast<float>(d[0]), static_cast<float>(d[1]), static_cast<float>(d[2])};
  Vec3<> rawAccel{static_cast<float>(d[3]), static_cast<float>(d[4]), static_cast<float>(d[5])};

  // Accel: remove offset, fix sensitivity, then counts -> m/s^2.
  out.accel = (rawAccel - accelBias_) * accelScale_ * (9.80665F / ACCEL_LSB_PER_G);

  // Gyro: remove bias, then counts -> deg/s.
  out.gyro = (rawGyro - gyroBias_) / GYRO_LSB_PER_DPS;
  return true;
}

// Sends 2 bytes: which register, then the value. True if the chip answered.
bool Imu::writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IMU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// Points the servos at the board's tilt, from accel only (no filter).
void movePos(int id, int position_number);
bool imuDriveMotors(Imu &imu) {
  ImuReading r;
  if (!imu.read(r)) {
    return false;
  }

  float pitch{atan2f(-r.accel.x, sqrt((r.accel.y * r.accel.y) + (r.accel.z * r.accel.z))) * 180.0 /
              std::numbers::pi};
  float roll{atan2f(r.accel.y, r.accel.z) * 180.0 / std::numbers::pi};

  // 4096 servo steps per turn, 2048 = center, clamped to +-90 degrees.
  movePos(1, constrain(2048 + (pitch * 4096.0 / 360.0), 1024, 3072));
  movePos(2, constrain(2048 + (roll * 4096.0 / 360.0), 1024, 3072));
  return true;
}

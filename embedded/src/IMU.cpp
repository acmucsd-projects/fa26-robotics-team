#include "IMU.hpp"
#include "BMI160.h"
#include "Vector.hpp"
#include <BMI160Gen.h>
#include <Wire.h>

int BMI160Class::serial_buffer_transfer(uint8_t *, unsigned, unsigned) { return 0; }

IMU::IMU(uint8_t sda_, uint8_t scl_, Vec3<float> accelBias_,
         Vec3<float> accelScale_, Vec3<float> gyroBias_)
    : sda{sda_}, scl{scl_}, accelBias{accelBias_}, accelScale{accelScale_},
      gyroBias{gyroBias_}, accelUnitConv{9.80665F / 8192.0F},
      gyroUnitConv{1.0F / 131.072F} {}

bool IMU::begin() const {
  Serial.begin(115200);
  while (!Serial) {
  }
  Wire.begin(sda, scl);
  Wire.setClock(100000);
  BMI160.setFullScaleGyroRange(BMI160_GYRO_RANGE_250);
  BMI160.setFullScaleAccelRange(BMI160_ACCEL_RANGE_4G);
  return BMI160.begin(BMI160GenClass::I2C_MODE, 0x68);
}

bool IMU::readTicks(IMUReading &out) {
  Vec3<int> rawAccelI{};
  Vec3<int> rawGyroI{};
  BMI160.readMotionSensor(rawAccelI.x, rawAccelI.y, rawAccelI.z, rawGyroI.x,
                          rawGyroI.y, rawGyroI.z);

  out.accel = (Vec3<float>{rawAccelI} - accelBias) * accelScale;
  out.gyro = Vec3<float>{rawGyroI} - gyroBias;
  return true;
}

bool IMU::read(IMUReading &out) {
  bool res{readTicks(out)};
  out.accel *= accelUnitConv;
  out.gyro *= gyroUnitConv;
  return res;
}

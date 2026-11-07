#include "IMU.hpp"
#include <BMI160Gen.h>
#include <Wire.h>

IMU imu{6, 7};
void setup() {
  if (!imu.begin()) {
    return;
  };
  Serial.println("BMI160 initialized successfully!");
}

void loop() {
  IMUReading r{};
  if (!imu.readTicks(r)) {
    return;
  }
  Serial.printf("ACC: %6d %6d %6d | GYR: %6d %6d %6d\n", r.accel.x, r.accel.y, r.accel.z, r.gyro.x,
                r.gyro.y, r.gyro.z);
  delay(100);
}

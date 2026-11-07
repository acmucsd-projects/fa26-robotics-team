#include "IMU.hpp"
#include "complimentary_filter.hpp"
#include <Arduino.h>

IMU imu{6, 7};
ComplementaryFilter filter;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    delay(10);
  }

  if (!imu.begin()) {
    Serial.println("BMI160 init failed");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("Calibrating gyro, keep the sensor still...");
  imu.calibrateGyro();
  Serial.println("BMI160 ready");
}

void loop() {
  IMUReading r{};
  if (!imu.read(r)) {
    Serial.println("read failed");
    delay(100);
    return;
  }

  static uint32_t last = micros();
  uint32_t now = micros();
  filter.update(r.accel.x, r.accel.y, r.accel.z, r.gyro.y, r.gyro.z, (now - last) / 1e6f);
  last = now;

  Serial.printf("accel (m/s^2): %7.2f %7.2f %7.2f  |a| %6.2f   gyro (deg/s): %8.2f %8.2f %8.2f   tilt %7.2f  pan %7.2f\n",
                r.accel.x, r.accel.y, r.accel.z,
                sqrtf(r.accel.x * r.accel.x + r.accel.y * r.accel.y + r.accel.z * r.accel.z),
                r.gyro.x, r.gyro.y, r.gyro.z, filter.tilt, filter.pan);
  delay(10);
}

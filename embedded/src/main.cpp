#include "imu.hpp"
#include <Arduino.h>

Imu imu(8, 9);

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
  ImuReading r;
  if (!imu.read(r)) {
    Serial.println("read failed");
    delay(100);
    return;
  }

  Serial.printf("accel (m/s^2): %7.2f %7.2f %7.2f  |a| %6.2f   gyro (deg/s): "
                "%8.2f %8.2f %8.2f\n",
                r.ax, r.ay, r.az, sqrtf(r.ax * r.ax + r.ay * r.ay + r.az * r.az),
                r.gx, r.gy, r.gz);
  delay(100);
}

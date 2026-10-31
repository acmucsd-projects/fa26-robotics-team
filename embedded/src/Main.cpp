/*
#include "Constants.hpp"
#include "IMU.hpp"
#include <Arduino.h>

IMU imu;

void setup() {
  imu = IMU{imuSDA, imuSCL};
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
  delay(100);
}
*/

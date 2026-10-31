#include "IMU.hpp"
#include <Arduino.h>

IMU imu{6, 7};
constexpr int nSample{1000};

void waitForEnter() {
  while (!Serial.available()) {
  }
}

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
  Serial.println("BMI160 ready");
}

void loop() {
  // Accel calibration
  std::array<const char *, 6> axes{"+x", "-x", "+y", "-y", "+z", "-z"};
}

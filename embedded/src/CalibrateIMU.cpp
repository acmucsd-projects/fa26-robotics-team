/* accel calibration, run once */
/* pio run -e calibrate -t upload -t monitor */
/* prints a line to paste into main.cpp */

#include "IMU.hpp"
#include <Arduino.h>

/* SDA & SCL */
Imu imu(6, 7);

void waitForEnter(const char *prompt) {
  while (Serial.available())
    Serial.read();
  while (!Serial.available()) {
    Serial.println(prompt);
    delay(2000);
  }
}

Vec3<float> measure(const char *prompt) {
  waitForEnter(prompt);
  Serial.println("   collecting samples bro... be patient");
  Vec3<float> gyro, accel;
  imu.averageRaw(gyro, accel, 420);
  Serial.println("  its done brochacho");
  return accel;
}

void setup() {
  Serial.begin(115200);
  if (!imu.begin()) {
    while (true) {
      Serial.println("init failed bruh");
      delay(1000);
    }
  }

  Vec3<float> xu =
      measure("hold it +X up, keep it there , press enter when up");
  Vec3<float> xd =
      measure("flip it so X points down dude, press enter when down");
  Vec3<float> yu = measure("place it +Y now dude, press enter when up");
  Vec3<float> yd =
      measure("flip it so Y points down dude, press enter when down");
  Vec3<float> zu = measure("place it +Z now dude, press enter when up");
  Vec3<float> zd =
      measure("flip it so Z points down dude, press enter when down");

  Vec3<float> up(xu.x, yu.y, zu.z);
  Vec3<float> down(xd.x, yd.y, zd.z);
  Vec3<float> offset = (up + down) / 2.0f;
  Vec3<float> scale = Vec3<float>(ACCEL_LSB_PER_G) / ((up - down) / 2.0f);

  Serial.println("calibration done, paste this into main.cpp:");
  Serial.printf(
      "Imu imu(6, 7, 0x68, {%.2f, %.2f, %.2f}, {%.4f, %.4f, %.4f});\n",
      offset.x, offset.y, offset.z, scale.x, scale.y, scale.z);
}

void loop() {}

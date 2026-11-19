// run this:
//   pio run -e calibrate -t upload -t monitor
// Point each axis up, then down. Gravity is exactly 1g, so the up/down
// readings tell us each axis's offset (middle) and scale (stretch).
// At the end it prints an `Imu imu(...)` line to paste into main.cpp.

#include "IMU.hpp"
#include <Arduino.h>

Imu imu;

// Repeats the prompt until something is typed in the serial monitor
void waitForEnter(const char *prompt) {
  while (Serial.available())
    Serial.read();
  while (!Serial.available()) {
    Serial.println(prompt);
    delay(2000); // During calibration, it stops "point _ up, then press enter"
                 // printing rapidly thousands of times
  }
}

// Waits for the user, then averages the accel while the board sits still
Vec3 measure(const char *prompt) {
  waitForEnter(prompt);
  Serial.println("  measuring, keep it still...");
  Vec3 gyro, accel;
  imu.averageRaw(gyro, accel, 400);
  Serial.println("  done");
  return accel;
}

void setup() {
  Serial.begin(115200);
  if (!imu.begin()) {
    while (true) {
      Serial.println("BMI160 init failed");
      delay(1000); // prevents "BMI160 init failed" from printing rapidly
                   // thousands of times
    }
  }

  Vec3 xu = measure("point +X up, then press enter");
  Vec3 xd = measure("point +X down, then press enter");
  Vec3 yu = measure("point +Y up, then press enter");
  Vec3 yd = measure("point +Y down, then press enter");
  Vec3 zu = measure("point +Z up, then press enter");
  Vec3 zd = measure("point +Z down, then press enter");

  // Each axis reads +1g up and -1g down. Offset = middle of the two,
  // scale = what turns half the gap into exactly 1g
  Vec3 up(xu.x, yu.y, zu.z);
  Vec3 down(xd.x, yd.y, zd.z);
  Vec3 offset = (up + down) / 2.0f;
  Vec3 scale = Vec3(ACCEL_LSB_PER_G) / ((up - down) / 2.0f);

  Serial.println("calibration done, paste this into main.cpp:");
  Serial.printf("Imu imu({%.2f, %.2f, %.2f}, {%.4f, %.4f, %.4f});\n", offset.x,
                offset.y, offset.z, scale.x, scale.y, scale.z);
}

void loop() {}

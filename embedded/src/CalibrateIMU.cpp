// run this:
//   pio run -e calibrate -t upload -t monitor
// Point each axis up, hold still, then down. Gravity is exactly 1g, so the up/down
// readings tell us each axis's offset (middle) and scale (stretch).
// At the end it prints an `Imu imu(...)` line to paste into main.cpp.

#include "IMU.hpp"
#include "Vector.hpp"
#include <Arduino.h>

Imu imu;

// Repeats the prompt until something is typed in the serial monitor
void waitForEnter(const char *prompt) {
  while (Serial.available()) {
    Serial.read();
  }

  while (!Serial.available()) {
    Serial.println(prompt);
    delay(2000); // During calibration, it stops "point _ up, hold still, then press enter"
                 // printing rapidly thousands of times
  }
}

// Waits for the user, hold still, then averages the accel while the board sits still
Vec3<> measure(const char *prompt, Vec3<> &gyro, Vec3<> &accel) {
  waitForEnter(prompt);
  Serial.println("  measuring, keep it still...");
  imu.avgRaw(gyro, accel, 400);
  Serial.println("  done");
  return accel;
}

void setup() {
  Serial.begin(115200);
  if (!imu.begin()) {
    while (true) {
      Serial.println("BMI160 init failed");
      delay(1000); // prevents "BMI160 init failed" from printing rapidly
    }
  }
  Vec3<> gyroBias;
  Vec3<> gyro;

  Vec3<> pX;
  measure("point +X up, hold still, then press enter", gyro, pX);
  gyroBias += gyro;

  Vec3<> nX;
  measure("point -X up, hold still, then press enter", gyro, nX);
  gyroBias += gyro;

  Vec3<> pY;
  measure("point +Y up, hold still, then press enter", gyro, pY);
  gyroBias += gyro;

  Vec3<> nY;
  measure("point -Y up, hold still, then press enter", gyro, nY);
  gyroBias += gyro;

  Vec3<> pZ;
  measure("point +Z up, hold still, then press enter", gyro, pZ);
  gyroBias += gyro;

  Vec3<> nZ;
  measure("point -Z up, hold still, then press enter", gyro, nZ);
  gyroBias += gyro;

  // Each axis reads +1g up and -1g down. Bias = middle of the two,
  // scale = what turns half the gap into exactly 1g
  Vec3<> positives{pX.x, pY.y, pZ.z};
  Vec3<> negatives{nX.x, nY.y, nZ.z};
  Vec3<> accelBias{(positives + negatives) / 2.0F};
  Vec3<> accelScale{Vec3<>{ACCEL_LSB_PER_G} / ((positives - negatives) / 2.0F)};

  Serial.println("calibration done, paste this into main.cpp:");
  Serial.printf("{%.4f, %.4f, %.4f}, {%.4f, %.4f, %.4f}, {%.4f, %.4f, %.4f}\n", gyroBias.x,
                gyroBias.y, gyroBias.z, accelBias.x, accelBias.y, accelBias.z, accelScale.x,
                accelScale.y, accelScale.z);
}

void loop() {}

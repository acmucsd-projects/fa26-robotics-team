// clang-format off
// Reads the IMU, runs the complementary filter, prints everything to serial.
// What you should see, one line per reading (board flat and still):
//
//   accel (m/s^2):    0.02   -0.05    9.80  |a|   9.80   gyro (deg/s):     0.01    -0.03     0.02   tilt   -0.12  pan    0.00
//                     \_____ x y z ______/    \ size /                     \______ x y z _______/          \ deg /       \ deg /
//
//   accel  gravity + motion along each axis. Flat: x,y ~ 0, z ~ 9.81.
//          Tip the board and gravity shifts between the axes.
//   |a|    total accel. Should stay ~ 9.81 when still. If not, recalibrate.
//   gyro   spin speed around each axis. ~ 0 when still, jumps when you turn.
//   tilt   filtered angle around Y (gyro y + accel). Tip the board, it follows.
//   pan    angle around Z (gyro z only). Turn the board flat, it follows,
//          but slowly drifts since nothing corrects it.
//
#include "IMU.hpp"
#include "Vector.hpp"
#include <Arduino.h>

Imu imu({4.5263, 5.8273, -3.0253}, {-7.7576, -510.4148, 91.7549}, {1.0080, -1.0080, 0.9991});

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  if (!imu.begin()) {
    Serial.println("BMI160 init failed");
    while (true) {
    }
  }
  Serial.println("BMI160 ready");
}

uint32_t last{};
void loop() {
  Vec3f gyro;
  Vec3f accel;
  if (!imu.read(gyro, accel)) {
    Serial.println("read failed");
    // Wait for the next IMU sample (100 Hz) so a dead IMU doesn't flood serial.
    delay(10);
    return;
  }

  if (millis() - last >= 10) {
    last = millis();
    Serial.printf("accel (m/s^2): %.4f %.4f %.4f. gyro (deg/s): %.4f %.4f %.4f\n",
                  accel.x(), accel.y(), accel.z(),
                  gyro.x(), gyro.y(), gyro.z());
  }
}

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
#include "ComplementaryFilter.hpp"
#include "IMU.hpp"
#include <Arduino.h>
#include "MQTT.cpp"

Imu imu({31.57, -498.17, 91.18}, {1.0106, 1.0074, 0.9943});
ComplementaryFilter filter;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  motorsBegin();

  // Setup: calls the mqtt server and designated port in MQTT.cpp
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  if (!imu.begin()) {
    Serial.println("BMI160 init failed");
    while (true) {
    }
  }
  Serial.println("Calibrating gyro, keep the sensor still...");
  imu.calibrateGyro();
  Serial.println("BMI160 ready");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  ImuReading r;
  if (!imu.read(r)) {
    Serial.println("read failed");
    // Wait for the next IMU sample (100 Hz) so a dead IMU doesn't flood serial.
    delay(10);
    return;
  }

  // dt = seconds since the last loop. The filter needs it to turn deg/s into deg.
  static uint32_t last = micros();
  uint32_t now = micros();
  filter.update(r, (now - last) / 1e6f);
  last = now;

  // Print at most every 10 ms so serial doesn't slow the loop down.
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 10) {
    lastPrint = millis();
    Serial.printf("accel (m/s^2): %7.2f %7.2f %7.2f  |a| %6.2f   gyro (deg/s): "
                  "%8.2f %8.2f %8.2f   tilt %7.2f  pan %7.2f\n",
                  r.accel.x, r.accel.y, r.accel.z,
                  sqrtf(r.accel.x * r.accel.x + r.accel.y * r.accel.y +
                        r.accel.z * r.accel.z),
                  r.gyro.x, r.gyro.y, r.gyro.z, filter.tilt, filter.pan); // gyro x y z
  }
}

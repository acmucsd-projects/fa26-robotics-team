// +x, -x, +y, -y, +z, -z calibration
// pio run -e calibrate -t upload -t monitor
#include "imu.hpp"
#include <Arduino.h>

constexpr int SAMPLES_PER_POSE = 420;

Imu imu(8, 9);

void waitForEnter(const char *prompt) {
  while (Serial.available())
    Serial.read();
  while (!Serial.available()) {
    Serial.println(prompt);
    for (int i = 0; i < 500 && !Serial.available(); i++)
      delay(10);
  }
  delay(50);
  while (Serial.available())
    Serial.read();
}

float measure(int axis, const char *prompt) {
  waitForEnter(prompt);
  Serial.println("   collecting samples bro... be patient");
  float avg[6];
  imu.averageRaw(avg, SAMPLES_PER_POSE);
  Serial.println("  its done brochacho");
  return avg[3 + axis];
}

void setup() {
  Serial.begin(115200);
  if (!imu.begin()) {
    while (true) {
      Serial.println("init failed bruh");
      delay(1000);
    }
  }

  const char *prompts[3][2] = {
      {"hold it +X up, keep it there , press enter when up",
       "place it -X now dude, press enter when up"},
      {"place it +Y now dude, press enter when up", "place it -Y now dude, press enter when up"},
      {"place it +Z now dude, press enter when up", "place it -Z now dude, press enter when up"},
  };

  float offset[3], scale[3];
  for (int axis = 0; axis < 3; axis++) {
    float up = measure(axis, prompts[axis][0]);
    float down = measure(axis, prompts[axis][1]);
    offset[axis] = (up + down) / 2;
    scale[axis] = ACCEL_LSB_PER_G / ((up - down) / 2);
  }

  imu.saveAccelCalibration(offset, scale);
  Serial.printf("calibration saved\n"
                "  accel offset (m/s^2): %7.3f %7.3f %7.3f\n"
                "  accel scale:          %7.4f %7.4f %7.4f\n",
                offset[0] * ACCEL_MS2_PER_LSB, offset[1] * ACCEL_MS2_PER_LSB,
                offset[2] * ACCEL_MS2_PER_LSB, scale[0], scale[1], scale[2]);
  Serial.println("flash main firmware now: pio run -t upload -t monitor");
}

void loop() {}

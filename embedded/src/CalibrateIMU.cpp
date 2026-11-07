/* accel calibration, run once */
/* pio run -e calibrate -t upload -t monitor */
/* hold the board with each axis pointing up then down, 6 poses total */
/* prints a line to paste into main.cpp */

#include "IMU.hpp"
#include <Arduino.h>

Imu imu(6, 7);

void waitForEnter(const char *prompt) {
  while (Serial.available())
    Serial.read();
  while (!Serial.available()) {
    Serial.println(prompt);
    delay(2000);
  }
}

/* waits for enter, then returns the averaged raw accel on that axis */
float measure(int axis, const char *prompt) {
  waitForEnter(prompt);
  Serial.println("   collecting samples bro... be patient");
  Vec3<float> gyro, accel;
  imu.averageRaw(gyro, accel, 420);
  Serial.println("  its done brochacho");
  return axis == 0 ? accel.x : axis == 1 ? accel.y : accel.z;
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
       "flip it so X points down dude, press enter when down"},
      {"place it +Y now dude, press enter when up",
       "flip it so Y points down dude, press enter when down"},
      {"place it +Z now dude, press enter when up",
       "flip it so Z points down dude, press enter when down"},
  };

  /* pointing up reads +1g, pointing down reads -1g */
  /* offset = 0g */
  /* scale = fixes the size, 8192 / the counts per g it actually measure */
  float offset[3], scale[3];
  for (int axis = 0; axis < 3; axis++) {
    float up = measure(axis, prompts[axis][0]);
    float down = measure(axis, prompts[axis][1]);
    offset[axis] = (up + down) / 2;
    scale[axis] = ACCEL_LSB_PER_G / ((up - down) / 2);
  }

  Serial.println("calibration done, paste this into main.cpp:");
  Serial.printf(
      "Imu imu(6, 7, 0x69, {%.2f, %.2f, %.2f}, {%.4f, %.4f, %.4f});\n",
      offset[0], offset[1], offset[2], scale[0], scale[1], scale[2]);
}

void loop() {}

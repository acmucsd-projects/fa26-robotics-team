/* accel calibration, run once */
/* pio run -e calibrate -t upload -t monitor */
/* hold the board with each axis pointing up then down, 6 poses total */
/* prints a line to paste into main.cpp */

#include "IMU.hpp"
#include <Arduino.h>

/* ~4s of readings per pose */
constexpr int SAMPLES_PER_POSE = 420;

Imu imu(6, 7);

/* clears old input, repeats the prompt every 5s until enter is pressed */
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

/* waits for enter, then returns the averaged raw accel on that axis */
/* avg is gyro 0-2 then accel 3-5, so accel x is avg[3] */
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
      {"place it +Y now dude, press enter when up",
       "place it -Y now dude, press enter when up"},
      {"place it +Z now dude, press enter when up",
       "place it -Z now dude, press enter when up"},
  };

  /* pointing up reads +1g, pointing down reads -1g */
  /* this is because we already did unit converstion in IMU.hpp*/
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

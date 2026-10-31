#pragma once
#include <math.h>

// tilt and pan
struct ComplementaryFilter {
  float alpha = 0.98f; // gyro trust per step, lower = more accel correction
  float tilt = 0;
  float pan = 0;

  void update(float ax, float ay, float az, float tiltRate, float panRate, float dt) {
    float accelTilt = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.29578f;

    // start from the accel angle instead of ramping up from 0
    if (!started) {
      tilt = accelTilt;
      started = true;
    } else {
      tilt = alpha * (tilt + tiltRate * dt) + (1 - alpha) * accelTilt;
    }

    pan += panRate * dt;
  }

private:
  bool started = false;
};

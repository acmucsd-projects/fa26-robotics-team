#pragma once
#include <math.h>

struct ComplementaryFilter {
  float alpha = 0.98f;
  float tilt = 0;
  float pan = 0;

  void update(float ax, float ay, float az, float tiltRate, float panRate, float dt) {
    float accelTilt = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.29578f;

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

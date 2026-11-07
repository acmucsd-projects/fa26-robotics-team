#pragma once
#include <math.h>

/* mixes gyro with accel */
struct ComplementaryFilter {
  /* 0.98 = 98% gyro 2% accel */
  /* higher = smoother but slower to fix drift, at ~100hz 0.98 fixes drift in
   * about 0.5s */
  float alpha = 0.98f;
  /* both in degrees */
  float tilt = 0;
  float pan = 0;

  /* ax ay az in m/s^2, tiltRate = gyro y and panRate = gyro z in deg/s */
  /* dt = seconds since the last update */
  void update(float ax, float ay, float az, float tiltRate, float panRate,
              float dt) {
    /* tilt angle from which way gravity points, only right when the board isnt
     * accelerating */
    /* 57.29578 turns radians into degrees */
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

#pragma once
#include <math.h>

// Complementary filter: blends gyro and accel into one steady angle.
//   gyro:  smooth and fast, but slowly drifts (we add up small errors forever)
//   accel: never drifts (gravity always points down), but jumps on every bump
// Each update trusts the gyro for quick moves and lets the accel slowly pull
// the angle back to the truth.
// Pan has no accel reference (gravity doesn't change when you turn), so it is
// gyro only and will drift.
struct ComplementaryFilter {
  // 0.98 = 98% gyro, 2% accel. Higher = smoother but slower to fix drift.
  // 0.98 fixes drift in about 50 loops, so a faster loop fixes it faster.
  float alpha = 0.98f;
  // Both in degrees.
  float tilt = 0;
  float pan = 0;

  // ax ay az in m/s^2, tiltRate = gyro y and panRate = gyro z in deg/s,
  // dt = seconds since the last update.
  void update(float ax, float ay, float az, float tiltRate, float panRate,
              float dt) {
    // Tilt seen by gravity alone. 57.29578 turns radians into degrees.
    float accelTilt = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.29578f;

    // First update: no history yet, so start from the accel angle.
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

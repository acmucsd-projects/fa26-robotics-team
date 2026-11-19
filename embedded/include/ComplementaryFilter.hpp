#pragma once
#include "IMU.hpp"
#include <math.h>

// Complementary filter: blends gyro and accel for angles
//   gyro:  smooth and fast, but slowly drifts (we add up small errors forever)
//   accel: never drifts (gravity always points down), but jumps on every bump
// Gyro for quick moves and lets the accel slowly pull he angle back.
struct ComplementaryFilter {
  // 0.98 = 98% gyro, 2% accel. Higher = smoother but slower to fix drift.
  // 0.98 fixes drift in about 50 loops, so a faster loop fixes it faster.
  float alpha = 0.98F;
  // Both in degrees.
  float tilt = 0;
  float pan = 0;

  // r = one IMU reading. Tilt uses gyro y, pan uses gyro z.
  // dt = seconds since the last update.
  void update(const ImuReading &r, float dt) {
    // Tilt seen by gravity alone. 57.29578 turns radians into degrees.
    float accelTilt{atan2f(-r.accel.x, sqrtf(r.accel.y * r.accel.y + r.accel.z * r.accel.z)) *
                    57.29578F};

    // no history yet, so start from the accel angle.
    if (!started_) {
      tilt = accelTilt;
      started_ = true;
    } else {
      tilt = alpha * (tilt + r.gyro.y * dt) + (1 - alpha) * accelTilt;
    }
    pan += r.gyro.z * dt;
  }

private:
  bool started_{};
};

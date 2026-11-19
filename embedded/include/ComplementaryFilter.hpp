#pragma once
#include "Vector.hpp"
#include <cmath>
#include <numbers>

// Complementary filter: blends gyro and accel for angles
//   gyro:  smooth and fast, but slowly drifts (we add up small errors forever)
//   accel: never drifts (gravity always points down), but jumps on every bump
// Gyro for quick moves and lets the accel slowly pull he angle back.
class ComplementaryFilter {
  // 0.98 = 98% gyro, 2% accel. Higher = smoother but slower to fix drift.
  // 0.98 fixes drift in about 50 loops, so a faster loop fixes it faste
  float alpha{0.98F};

  // Both in degrees.
  float tilt{};
  float pan{};

  // r = one IMU reading. Tilt uses gyro y, pan uses gyro z.
  // dt = seconds since the last update.
  void update(Vec3<> gyro, Vec3<> accel, float dt) {
    // Tilt seen by gravity alone. 57.29578 turns radians into degrees.
    float accelTilt{atan2f(-accel.x, sqrtf((accel.y * accel.y) + (accel.z * accel.z))) *
                    (180.0F / std::numbers::pi)};

    // No history yet, so start from the accel angle.
    if (!started_) {
      tilt = accelTilt;
      started_ = true;
    } else [[likely]] {
      tilt = (alpha * (tilt + (gyro.y * dt))) + ((1 - alpha) * accelTilt);
    }
    pan += gyro.z * dt;
  }

private:
  bool started_{};
};

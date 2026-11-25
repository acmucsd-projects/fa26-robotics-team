#pragma once

// 3 floats with component-wise math. A plain number turns into (n, n, n), so
// v * 2 and v / 2 work too.
struct Vec3 {
  float x;
  float y;
  float z;
  Vec3() = default;
  Vec3(float s) : x{s}, y{s}, z{s} {}
  Vec3(float x_, float y_, float z_) : x{x_}, y{y_}, z{z_} {}
  Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
  Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
  Vec3 operator*(const Vec3 &o) const { return {x * o.x, y * o.y, z * o.z}; }
  Vec3 operator/(const Vec3 &o) const { return {x / o.x, y / o.y, z / o.z}; }
};

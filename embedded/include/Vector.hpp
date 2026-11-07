#pragma once
#include <concepts>
#include <type_traits>

template <typename T>
  requires std::is_arithmetic_v<T>
struct Vec3 {
  T x;
  T y;
  T z;
  Vec3() = default;
  Vec3(T scalar) : x{scalar}, y{scalar}, z{scalar} {}
  Vec3(T x_, T y_, T z_) : x{x_}, y{y_}, z{z_} {}
  Vec3<T> &operator+=(const Vec3<T> &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  Vec3<T> &operator-=(const Vec3<T> &other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }
  Vec3<T> &operator*=(const Vec3<T> &other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }
  Vec3<T> &operator/=(const Vec3<T> &other) {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
  }

  Vec3<T> &operator*=(T scalar) {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }
  Vec3<T> &operator/=(T scalar) {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
  }
  Vec3<T> operator+(const Vec3<T> &other) const { return Vec3(*this) += other; }
  Vec3<T> operator-(const Vec3<T> &other) const { return Vec3(*this) -= other; }
  Vec3<T> operator*(const Vec3<T> &other) const { return Vec3(*this) *= other; }
  Vec3<T> operator/(const Vec3<T> &other) const { return Vec3(*this) /= other; }
  Vec3<T> operator*(T scalar) const { return Vec3(*this) *= scalar; }
  Vec3<T> operator/(T scalar) const { return Vec3(*this) /= scalar; }
  template <typename T2>
    requires std::convertible_to<T, T2>
  operator Vec3<T2>() {
    Vec3<T2> other;
    other.x = static_cast<T2>(x);
    other.y = static_cast<T2>(y);
    other.z = static_cast<T2>(z);
    return other;
  }
};

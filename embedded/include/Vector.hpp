#pragma once
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

template <typename T = float>
  requires std::is_arithmetic_v<T>
struct Vec3 {
  T x;
  T y;
  T z;

  constexpr Vec3() noexcept = default;
  constexpr Vec3(T scalar) noexcept : x{scalar}, y{scalar}, z{scalar} {}
  constexpr Vec3(T x_, T y_, T z_) noexcept : x{x_}, y{y_}, z{z_} {}

  [[nodiscard]] constexpr T &operator[](size_t idx) noexcept {
    switch (idx) {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      assert(false);
    }
  }

  [[nodiscard]] constexpr const T &operator[](size_t idx) const noexcept {
    switch (idx) {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      assert(false);
    }
  }

  constexpr Vec3<T> &operator+=(const Vec3<T> &other) noexcept {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  constexpr Vec3<T> &operator-=(const Vec3<T> &other) noexcept
    requires std::is_signed_v<T>
  {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }

  constexpr Vec3<T> &operator*=(const Vec3<T> &other) noexcept {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }

  constexpr Vec3<T> &operator/=(const Vec3<T> &other) noexcept {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
  }

  constexpr Vec3<T> &operator*=(T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }

  constexpr Vec3<T> &operator/=(T scalar) noexcept {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
  }

  [[nodiscard]] constexpr Vec3<T> operator+(const Vec3<T> &other) const noexcept {
    return Vec3(*this) += other;
  }

  [[nodiscard]] constexpr Vec3<T> operator-(const Vec3<T> &other) const noexcept {
    return Vec3(*this) -= other;
  }

  [[nodiscard]] constexpr Vec3<T> operator*(const Vec3<T> &other) const noexcept {
    return Vec3(*this) *= other;
  }

  [[nodiscard]] constexpr Vec3<T> operator/(const Vec3<T> &other) const noexcept {
    return Vec3(*this) /= other;
  }

  [[nodiscard]] constexpr Vec3<T> operator*(T scalar) const noexcept {
    return Vec3(*this) *= scalar;
  }

  [[nodiscard]] constexpr Vec3<T> operator/(T scalar) const noexcept {
    return Vec3(*this) /= scalar;
  }

  [[nodiscard]] constexpr Vec3<T> operator-() const noexcept { return Vec3<T>{-x, -y, -z}; }

  template <typename T2>
    requires std::convertible_to<T, T2>
  [[nodiscard]] constexpr operator Vec3<T2>() const noexcept {
    return Vec3<T2>{static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z)};
  }

  // Linear algebra helpers
  [[nodiscard]] constexpr T dot(const Vec3<T> &other) const noexcept {
    return (x * other.x) + (y * other.y) + (z * other.z);
  }

  [[nodiscard]] constexpr Vec3<T> cross(const Vec3<T> &other) const noexcept {
    return Vec3<T>{
        (y * other.z) - (z * other.y),
        (z * other.x) - (x * other.z),
        (x * other.y) - (y * other.x),
    };
  }

  [[nodiscard]] constexpr T lengthSqrd() const noexcept { return dot(*this); }

  [[nodiscard]] constexpr T length() const noexcept
    requires std::floating_point<T>
  {
    return std::sqrt(lengthSqrd());
  }

  [[nodiscard]] constexpr Vec3<T> normalized() const noexcept
    requires std::floating_point<T>
  {
    const T len{length()};
    if (len <= T{0}) {
      return Vec3<T>{};
    }
    return *this / len;
  }
};

template <typename T>
  requires std::is_arithmetic_v<T>
[[nodiscard]] constexpr Vec3<T> operator*(T scalar, const Vec3<T> &vec) noexcept {
  return vec * scalar;
}

template <typename T>
  requires std::is_arithmetic_v<T>
[[nodiscard]] constexpr Vec3<T> operator/(T scalar, const Vec3<T> &vec) noexcept {
  return Vec3<T>{scalar / vec.x, scalar / vec.y, scalar / vec.z};
}

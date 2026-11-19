#pragma once
#include "Array.hpp"
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

// Fixed-size mathematical vector
template <typename T, size_t N>
  requires std::is_arithmetic_v<T> && (N > 0)
struct Vec {
  Array<T, N> data;

  constexpr Vec() noexcept = default;

  constexpr explicit Vec(T scalar) noexcept {
    for (size_t i{}; i < N; ++i) {
      data[i] = scalar;
    }
  }

  template <typename... Ts>
    requires(sizeof...(Ts) == N) && (std::convertible_to<Ts, T> && ...)
  constexpr Vec(Ts... vals) noexcept : data{static_cast<T>(vals)...} {}

  // Named component accessors for 1D-4D convenience
  [[nodiscard]] constexpr T &x() noexcept
    requires(N >= 1)
  {
    return data[0];
  }
  [[nodiscard]] constexpr const T &x() const noexcept
    requires(N >= 1)
  {
    return data[0];
  }

  [[nodiscard]] constexpr T &y() noexcept
    requires(N >= 2)
  {
    return data[1];
  }
  [[nodiscard]] constexpr const T &y() const noexcept
    requires(N >= 2)
  {
    return data[1];
  }

  [[nodiscard]] constexpr T &z() noexcept
    requires(N >= 3)
  {
    return data[2];
  }
  [[nodiscard]] constexpr const T &z() const noexcept
    requires(N >= 3)
  {
    return data[2];
  }

  [[nodiscard]] constexpr T &w() noexcept
    requires(N >= 4)
  {
    return data[3];
  }
  [[nodiscard]] constexpr const T &w() const noexcept
    requires(N >= 4)
  {
    return data[3];
  }

  [[nodiscard]] constexpr T &operator[](size_t idx) noexcept {
    assert(idx < N && "Vec index out of range");
    return data[idx];
  }

  [[nodiscard]] constexpr const T &operator[](size_t idx) const noexcept {
    assert(idx < N && "Vec index out of range");
    return data[idx];
  }

  constexpr Vec &operator+=(const Vec &other) noexcept {
    for (size_t i{}; i < N; ++i) {
      data[i] += other.data[i];
    }
    return *this;
  }

  constexpr Vec &operator-=(const Vec &other) noexcept
    requires std::is_signed_v<T>
  {
    for (size_t i{}; i < N; ++i) {
      data[i] -= other.data[i];
    }
    return *this;
  }

  constexpr Vec &operator*=(const Vec &other) noexcept {
    for (size_t i{}; i < N; ++i) {
      data[i] *= other.data[i];
    }
    return *this;
  }

  constexpr Vec &operator/=(const Vec &other) noexcept {
    for (size_t i{}; i < N; ++i) {
      data[i] /= other.data[i];
    }
    return *this;
  }

  constexpr Vec &operator*=(T scalar) noexcept {
    for (size_t i{}; i < N; ++i) {
      data[i] *= scalar;
    }
    return *this;
  }

  constexpr Vec &operator/=(T scalar) noexcept {
    for (size_t i{}; i < N; ++i) {
      data[i] /= scalar;
    }
    return *this;
  }

  [[nodiscard]] constexpr Vec operator+(const Vec &other) const noexcept {
    return Vec(*this) += other;
  }

  [[nodiscard]] constexpr Vec operator-(const Vec &other) const noexcept
    requires std::is_signed_v<T>
  {
    return Vec(*this) -= other;
  }

  [[nodiscard]] constexpr Vec operator*(const Vec &other) const noexcept {
    return Vec(*this) *= other;
  }

  [[nodiscard]] constexpr Vec operator/(const Vec &other) const noexcept {
    return Vec(*this) /= other;
  }

  [[nodiscard]] constexpr Vec operator*(T scalar) const noexcept { return Vec(*this) *= scalar; }

  [[nodiscard]] constexpr Vec operator/(T scalar) const noexcept { return Vec(*this) /= scalar; }

  [[nodiscard]] constexpr Vec operator-() const noexcept
    requires std::is_signed_v<T>
  {
    Vec res;
    for (size_t i{}; i < N; ++i) {
      res[i] = -data[i];
    }
    return res;
  }

  template <typename T2>
    requires std::is_arithmetic_v<T2> && std::convertible_to<T, T2>
  [[nodiscard]] constexpr explicit operator Vec<T2, N>() const noexcept {
    Vec<T2, N> res{};
    for (size_t i{}; i < N; ++i) {
      res[i] = static_cast<T2>(data[i]);
    }
    return res;
  }

  [[nodiscard]] constexpr T dot(const Vec &other) const noexcept {
    T sum{T{}};
    for (size_t i{}; i < N; ++i) {
      sum += data[i] * other.data[i];
    }
    return sum;
  }

  [[nodiscard]] constexpr Vec cross(const Vec &other) const noexcept
    requires(N == 3)
  {
    return Vec{
        (data[1] * other.data[2]) - (data[2] * other.data[1]),
        (data[2] * other.data[0]) - (data[0] * other.data[2]),
        (data[0] * other.data[1]) - (data[1] * other.data[0]),
    };
  }

  [[nodiscard]] constexpr T lengthSqrd() const noexcept { return dot(*this); }

  [[nodiscard]] T length() const noexcept
    requires std::floating_point<T>
  {
    return std::sqrt(lengthSqrd());
  }

  [[nodiscard]] Vec normalized() const noexcept
    requires std::floating_point<T>
  {
    const T len{length()};
    if (len <= T{}) {
      return Vec{};
    }
    return *this / len;
  }
};

template <typename T, size_t N>
  requires std::is_arithmetic_v<T> && (N > 0)
[[nodiscard]] constexpr Vec<T, N> operator*(T scalar, const Vec<T, N> &vec) noexcept {
  return vec * scalar;
}

template <typename T, size_t N>
  requires std::is_arithmetic_v<T> && (N > 0)
[[nodiscard]] constexpr Vec<T, N> operator/(T scalar, const Vec<T, N> &vec) noexcept {
  Vec<T, N> res{};
  for (size_t i{}; i < N; ++i) {
    res[i] = scalar / vec[i];
  }
  return res;
}

using Vec3f = Vec<float, 3>;

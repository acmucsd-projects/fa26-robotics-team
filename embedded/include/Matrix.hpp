#pragma once
#include "Array.hpp"
#include "Vector.hpp"
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>

template <typename T = float>
  requires std::is_arithmetic_v<T>
struct Mat3 {
  // Row-major storage: rows[r][c]
  Array<Vec3<T>, 3> rows{
      Vec3<T>{T{1}, T{0}, T{0}},
      Vec3<T>{T{0}, T{1}, T{0}},
      Vec3<T>{T{0}, T{0}, T{1}},
  };

  constexpr Mat3() noexcept = default;

  // Diagonal matrix constructor (e.g., Mat3{0.0F} for zero matrix, Mat3{2.0F} for 2*I)
  constexpr explicit Mat3(T diag) noexcept
      : rows{
            Vec3<T>{diag, T{0}, T{0}},
            Vec3<T>{T{0}, diag, T{0}},
            Vec3<T>{T{0}, T{0}, diag},
        } {}

  constexpr Mat3(const Vec3<T> &r0, const Vec3<T> &r1, const Vec3<T> &r2) noexcept
      : rows{r0, r1, r2} {}

  constexpr Mat3(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22) noexcept
      : rows{
            Vec3<T>{m00, m01, m02},
            Vec3<T>{m10, m11, m12},
            Vec3<T>{m20, m21, m22},
        } {}

  // Static constructors
  [[nodiscard]] static constexpr Mat3<T> zero() noexcept { return Mat3<T>{T{0}}; }

  [[nodiscard]] static constexpr Mat3<T> identity() noexcept { return Mat3<T>{}; }

  [[nodiscard]] static constexpr Mat3<T> rotX(T rad) noexcept {
    const T c{std::cos(rad)};
    const T s{std::sin(rad)};
    return Mat3<T>{
        Vec3<T>{T{1}, T{0}, T{0}},
        Vec3<T>{T{0}, c, -s},
        Vec3<T>{T{0}, s, c},
    };
  }

  [[nodiscard]] static constexpr Mat3<T> rotY(T rad) noexcept {
    const T c{std::cos(rad)};
    const T s{std::sin(rad)};
    return Mat3<T>{
        Vec3<T>{c, T{0}, s},
        Vec3<T>{T{0}, T{1}, T{0}},
        Vec3<T>{-s, T{0}, c},
    };
  }

  [[nodiscard]] static constexpr Mat3<T> rotZ(T rad) noexcept {
    const T c{std::cos(rad)};
    const T s{std::sin(rad)};
    return Mat3<T>{
        Vec3<T>{c, -s, T{0}},
        Vec3<T>{s, c, T{0}},
        Vec3<T>{T{0}, T{0}, T{1}},
    };
  }

  [[nodiscard]] constexpr Vec3<T> &operator[](size_t r) noexcept { return rows[r]; }
  [[nodiscard]] constexpr const Vec3<T> &operator[](size_t r) const noexcept { return rows[r]; }

  [[nodiscard]] constexpr Vec3<T> col(size_t c) const noexcept {
    assert(c < 3);
    return Vec3<T>{rows[0][c], rows[1][c], rows[2][c]};
  }

  [[nodiscard]] constexpr Mat3<T> operator-() const noexcept {
    return Mat3<T>{-rows[0], -rows[1], -rows[2]};
  }

  constexpr Mat3<T> &operator+=(const Mat3<T> &other) noexcept {
    rows[0] += other.rows[0];
    rows[1] += other.rows[1];
    rows[2] += other.rows[2];
    return *this;
  }

  constexpr Mat3<T> &operator-=(const Mat3<T> &other) noexcept {
    rows[0] -= other.rows[0];
    rows[1] -= other.rows[1];
    rows[2] -= other.rows[2];
    return *this;
  }

  constexpr Mat3<T> &operator*=(const Mat3<T> &rhs) noexcept {
    *this = (*this) * rhs;
    return *this;
  }

  constexpr Mat3<T> &operator*=(T scalar) noexcept {
    rows[0] *= scalar;
    rows[1] *= scalar;
    rows[2] *= scalar;
    return *this;
  }

  constexpr Mat3<T> &operator/=(T scalar) noexcept {
    rows[0] /= scalar;
    rows[1] /= scalar;
    rows[2] /= scalar;
    return *this;
  }

  // Binary arithmetic (Matrix-Matrix)
  [[nodiscard]] constexpr Mat3<T> operator+(const Mat3<T> &other) const noexcept {
    return Mat3<T>(*this) += other;
  }

  [[nodiscard]] constexpr Mat3<T> operator-(const Mat3<T> &other) const noexcept {
    return Mat3<T>(*this) -= other;
  }

  [[nodiscard]] constexpr Mat3<T> operator*(const Mat3<T> &rhs) const noexcept {
    const Vec3<T> c0{rhs.col(0)};
    const Vec3<T> c1{rhs.col(1)};
    const Vec3<T> c2{rhs.col(2)};
    return Mat3<T>{
        Vec3<T>{rows[0].dot(c0), rows[0].dot(c1), rows[0].dot(c2)},
        Vec3<T>{rows[1].dot(c0), rows[1].dot(c1), rows[1].dot(c2)},
        Vec3<T>{rows[2].dot(c0), rows[2].dot(c1), rows[2].dot(c2)},
    };
  }

  // Matrix * Vector (column)
  [[nodiscard]] constexpr Vec3<T> operator*(const Vec3<T> &v) const noexcept {
    return Vec3<T>{rows[0].dot(v), rows[1].dot(v), rows[2].dot(v)};
  }

  // Binary arithmetic (Scalar)
  [[nodiscard]] constexpr Mat3<T> operator*(T scalar) const noexcept {
    return Mat3<T>(*this) *= scalar;
  }

  [[nodiscard]] constexpr Mat3<T> operator/(T scalar) const noexcept {
    return Mat3<T>(*this) /= scalar;
  }

  template <typename T2>
    requires std::convertible_to<T, T2>
  [[nodiscard]] constexpr operator Mat3<T2>() const noexcept {
    return Mat3<T2>{
        static_cast<Vec3<T2>>(rows[0]),
        static_cast<Vec3<T2>>(rows[1]),
        static_cast<Vec3<T2>>(rows[2]),
    };
  }

  // Linear algebra helpers
  [[nodiscard]] constexpr Mat3<T> transpose() const noexcept {
    return Mat3<T>{col(0), col(1), col(2)};
  }

  [[nodiscard]] constexpr T det() const noexcept {
    return rows[0].x * (rows[1].y * rows[2].z - rows[1].z * rows[2].y) -
           rows[0].y * (rows[1].x * rows[2].z - rows[1].z * rows[2].x) +
           rows[0].z * (rows[1].x * rows[2].y - rows[1].y * rows[2].x);
  }

  // General inverse
  [[nodiscard]] constexpr Mat3<T> inv() const noexcept {
    const T d{det()};
    assert(d != T{0});
    const T invDet{T{1} / d};
    return Mat3<T>{
        Vec3<T>{
            (rows[1].y * rows[2].z - rows[1].z * rows[2].y) * invDet,
            (rows[0].z * rows[2].y - rows[0].y * rows[2].z) * invDet,
            (rows[0].y * rows[1].z - rows[0].z * rows[1].y) * invDet,
        },
        Vec3<T>{
            (rows[1].z * rows[2].x - rows[1].x * rows[2].z) * invDet,
            (rows[0].x * rows[2].z - rows[0].z * rows[2].x) * invDet,
            (rows[0].z * rows[1].x - rows[0].x * rows[1].z) * invDet,
        },
        Vec3<T>{
            (rows[1].x * rows[2].y - rows[1].y * rows[2].x) * invDet,
            (rows[0].y * rows[2].x - rows[0].x * rows[2].y) * invDet,
            (rows[0].x * rows[1].y - rows[0].y * rows[1].x) * invDet,
        },
    };
  }
};

// Left-hand scalar multiplication (e.g., 2.0F * mat)
template <typename T>
  requires std::is_arithmetic_v<T>
[[nodiscard]] constexpr Mat3<T> operator*(T scalar, const Mat3<T> &mat) noexcept {
  return mat * scalar;
}

// Vector (row) * Matrix
template <typename T>
  requires std::is_arithmetic_v<T>
[[nodiscard]] constexpr Vec3<T> operator*(const Vec3<T> &v, const Mat3<T> &mat) noexcept {
  return Vec3<T>{v.dot(mat.col(0)), v.dot(mat.col(1)), v.dot(mat.col(2))};
}

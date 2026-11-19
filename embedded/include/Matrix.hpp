#pragma once
#include "Array.hpp"
#include "Vector.hpp"
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <numbers>
#include <type_traits>

// Matrix with fixed rows and columns, with implementations stolen from Eigen
template <typename T, size_t R, size_t C>
  requires std::is_arithmetic_v<T> && (R > 0) && (C > 0)
struct Mat {
  // Row-major storage: rows[r][c]
  Array<Vec<T, C>, R> rows;

  // Default constructs to Identity when square (R == C), Zero otherwise
  constexpr Mat() noexcept {
    if constexpr (R == C) {
      for (size_t i{}; i < R; ++i) {
        rows[i][i] = T{1};
      }
    }
  }

  // Diagonal matrix constructor (square matrices only)
  constexpr explicit Mat(T diag) noexcept
    requires(R == C)
  {
    for (size_t i{}; i < R; ++i) {
      rows[i][i] = diag;
    }
  }

  // Construct from R row vectors
  template <typename... RowVecs>
    requires(sizeof...(RowVecs) == R) &&
            (std::same_as<std::remove_cvref_t<RowVecs>, Vec<T, C>> && ...)
  constexpr Mat(RowVecs &&...rowVecs) noexcept : rows{static_cast<RowVecs &&>(rowVecs)...} {}

  // Construct from R * C flat elements in row-major order
  template <typename... Elems>
    requires(sizeof...(Elems) == R * C) && (R * C > 1) && (std::convertible_to<Elems, T> && ...)
  constexpr Mat(Elems... elems) noexcept {
    const Array<T, R * C> flat{static_cast<T>(elems)...};
    for (size_t r{}; r < R; ++r) {
      for (size_t c{}; c < C; ++c) {
        rows[r][c] = flat[(r * C) + c];
      }
    }
  }

  // Static constructors
  [[nodiscard]] static constexpr Mat zero() noexcept {
    Mat res{};
    for (size_t r{}; r < R; ++r) {
      res.rows[r] = Vec<T, C>{};
    }
    return res;
  }

  [[nodiscard]] static constexpr Mat identity() noexcept
    requires(R == C)
  {
    return Mat{};
  }

  [[nodiscard]] static constexpr Mat rotX(T deg) noexcept
    requires(R == 3 && C == 3) && std::floating_point<T> && std::numbers::pi_v<T>
  {
    T rad{deg * std::numbers::pi_v<T> / T{180}};
    const T c{std::cos(rad)};
    const T s{std::sin(rad)};
    return Mat{
        Vec<T, 3>{T{1}, T{}, T{}},
        Vec<T, 3>{T{}, c, -s},
        Vec<T, 3>{T{}, s, c},
    };
  }

  [[nodiscard]] static constexpr Mat rotY(T deg) noexcept
    requires(R == 3 && C == 3) && std::floating_point<T> && std::numbers::pi_v<T>
  {
    T rad{deg * std::numbers::pi_v<T> / T{180}};
    const T c{std::cos(rad)};
    const T s{std::sin(rad)};
    return Mat{
        Vec<T, 3>{c, T{}, s},
        Vec<T, 3>{T{}, T{1}, T{}},
        Vec<T, 3>{-s, T{}, c},
    };
  }

  [[nodiscard]] static constexpr Mat rotZ(T deg) noexcept
    requires(R == 3 && C == 3) && std::floating_point<T> && std::numbers::pi_v<T>
  {
    T rad{deg * std::numbers::pi_v<T> / T{180}};
    const T c{std::cos(rad)};
    const T s{std::sin(rad)};
    return Mat{
        Vec<T, 3>{c, -s, T{}},
        Vec<T, 3>{s, c, T{}},
        Vec<T, 3>{T{}, T{}, T{1}},
    };
  }

  [[nodiscard]] constexpr Vec<T, C> &operator[](size_t r) noexcept {
    assert(r < R && "Mat row index out of range.");
    return rows[r];
  }

  [[nodiscard]] constexpr const Vec<T, C> &operator[](size_t r) const noexcept {
    assert(r < R && "Mat row index out of range.");
    return rows[r];
  }

  [[nodiscard]] constexpr Vec<T, R> col(size_t c) const noexcept {
    assert(c < C && "Mat column index out of range.");
    Vec<T, R> res{};
    for (size_t r{}; r < R; ++r) {
      res[r] = rows[r][c];
    }
    return res;
  }

  [[nodiscard]] constexpr Mat operator-() const noexcept
    requires std::is_signed_v<T>
  {
    Mat res{};
    for (size_t r{}; r < R; ++r) {
      res.rows[r] = -rows[r];
    }
    return res;
  }

  constexpr Mat &operator+=(const Mat &other) noexcept {
    for (size_t r{}; r < R; ++r) {
      rows[r] += other.rows[r];
    }
    return *this;
  }

  constexpr Mat &operator-=(const Mat &other) noexcept
    requires std::is_signed_v<T>
  {
    for (size_t r{}; r < R; ++r) {
      rows[r] -= other.rows[r];
    }
    return *this;
  }

  // Compound matrix multiplication is only valid when rhs is C x C
  constexpr Mat &operator*=(const Mat<T, C, C> &rhs) noexcept {
    *this = (*this) * rhs;
    return *this;
  }

  constexpr Mat &operator*=(T scalar) noexcept {
    for (size_t r{}; r < R; ++r) {
      rows[r] *= scalar;
    }
    return *this;
  }

  constexpr Mat &operator/=(T scalar) noexcept {
    for (size_t r{}; r < R; ++r) {
      rows[r] /= scalar;
    }
    return *this;
  }

  // Binary arithmetic (Matrix-Matrix)
  [[nodiscard]] constexpr Mat operator+(const Mat &other) const noexcept {
    return Mat(*this) += other;
  }

  [[nodiscard]] constexpr Mat operator-(const Mat &other) const noexcept
    requires std::is_signed_v<T>
  {
    return Mat(*this) -= other;
  }

  // (R x C) * (C x K) -> (R x K)
  template <size_t K>
    requires(K > 0)
  [[nodiscard]] constexpr Mat<T, R, K> operator*(const Mat<T, C, K> &rhs) const noexcept {
    Mat<T, R, K> res{Mat<T, R, K>::zero()};
    for (size_t k{}; k < K; ++k) {
      const Vec<T, C> rhsCol{rhs.col(k)};
      for (size_t r{}; r < R; ++r) {
        res[r][k] = rows[r].dot(rhsCol);
      }
    }
    return res;
  }

  // (R x C) * (C x 1 column vector) -> (R x 1 column vector)
  [[nodiscard]] constexpr Vec<T, R> operator*(const Vec<T, C> &v) const noexcept {
    Vec<T, R> res{};
    for (size_t r{}; r < R; ++r) {
      res[r] = rows[r].dot(v);
    }
    return res;
  }

  // Binary arithmetic (Scalar)
  [[nodiscard]] constexpr Mat operator*(T scalar) const noexcept { return Mat(*this) *= scalar; }

  [[nodiscard]] constexpr Mat operator/(T scalar) const noexcept { return Mat(*this) /= scalar; }

  template <typename T2>
    requires std::is_arithmetic_v<T2> && std::convertible_to<T, T2>
  [[nodiscard]] constexpr explicit operator Mat<T2, R, C>() const noexcept {
    Mat<T2, R, C> res{};
    for (size_t r{}; r < R; ++r) {
      res.rows[r] = static_cast<Vec<T2, C>>(rows[r]);
    }
    return res;
  }

  // Transpose: (R x C) -> (C x R)
  [[nodiscard]] constexpr Mat<T, C, R> transpose() const noexcept {
    Mat<T, C, R> res{};
    for (size_t c{}; c < C; ++c) {
      res.rows[c] = col(c);
    }
    return res;
  }

  // Determinant (1x1, 2x2, 3x3 closed-form; NxN via Gauss-Jordan elimination)
  [[nodiscard]] constexpr T det() const noexcept
    requires(R == C)
  {
    if constexpr (R == 1) {
      return rows[0][0];
    } else if constexpr (R == 2) {
      return (rows[0][0] * rows[1][1]) - (rows[0][1] * rows[1][0]);
    } else if constexpr (R == 3) {
      return rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]) -
             rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0]) +
             rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
    } else {
      Mat a{*this};
      T determinant{T{1}};
      for (size_t i{}; i < R; ++i) {
        size_t pivot{i};
        for (size_t r{i + 1}; r < R; ++r) {
          const T absR{a[r][i] < T{} ? -a[r][i] : a[r][i]};
          const T absP{a[pivot][i] < T{} ? -a[pivot][i] : a[pivot][i]};
          if (absR > absP) {
            pivot = r;
          }
        }
        if (a[pivot][i] == T{}) {
          return T{};
        }
        if (pivot != i) {
          const Vec<T, C> tmp{a[i]};
          a[i] = a[pivot];
          a[pivot] = tmp;
          determinant = -determinant;
        }
        determinant *= a[i][i];
        for (size_t r{i + 1}; r < R; ++r) {
          const T factor{a[r][i] / a[i][i]};
          for (size_t c{i}; c < C; ++c) {
            a[r][c] -= factor * a[i][c];
          }
        }
      }
      return determinant;
    }
  }

  // General inverse (closed-form for <= 3x3; Gauss-Jordan with partial pivoting for NxN)
  [[nodiscard]] constexpr Mat inv() const noexcept
    requires(R == C) && std::floating_point<T>
  {
    if constexpr (R == 1) {
      assert(rows[0][0] != T{});
      return Mat{T{1} / rows[0][0]};
    } else if constexpr (R == 2) {
      const T d{det()};
      assert(d != T{});
      const T invDet{T{1} / d};
      return Mat{
          rows[1][1] * invDet,
          -rows[0][1] * invDet,
          -rows[1][0] * invDet,
          rows[0][0] * invDet,
      };
    } else if constexpr (R == 3) {
      const T d{det()};
      assert(d != T{});
      const T invDet{T{1} / d};
      return Mat{
          Vec<T, 3>{
              (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]) * invDet,
              (rows[0][2] * rows[2][1] - rows[0][1] * rows[2][2]) * invDet,
              (rows[0][1] * rows[1][2] - rows[0][2] * rows[1][1]) * invDet,
          },
          Vec<T, 3>{
              (rows[1][2] * rows[2][0] - rows[1][0] * rows[2][2]) * invDet,
              (rows[0][0] * rows[2][2] - rows[0][2] * rows[2][0]) * invDet,
              (rows[0][2] * rows[1][0] - rows[0][0] * rows[1][2]) * invDet,
          },
          Vec<T, 3>{
              (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]) * invDet,
              (rows[0][1] * rows[2][0] - rows[0][0] * rows[2][1]) * invDet,
              (rows[0][0] * rows[1][1] - rows[0][1] * rows[1][0]) * invDet,
          },
      };
    } else {
      Mat a{*this};
      Mat aug{Mat::identity()};
      for (size_t i{}; i < R; ++i) {
        size_t pivot{i};
        for (size_t r{i + 1}; r < R; ++r) {
          const T absR{a[r][i] < T{} ? -a[r][i] : a[r][i]};
          const T absP{a[pivot][i] < T{} ? -a[pivot][i] : a[pivot][i]};
          if (absR > absP) {
            pivot = r;
          }
        }
        assert(a[pivot][i] != T{} && "Singular matrix cannot be inverted");
        if (pivot != i) {
          const Vec<T, C> tmpA{a[i]};
          a[i] = a[pivot];
          a[pivot] = tmpA;

          const Vec<T, C> tmpAug{aug[i]};
          aug[i] = aug[pivot];
          aug[pivot] = tmpAug;
        }
        const T diag{a[i][i]};
        a[i] /= diag;
        aug[i] /= diag;
        for (size_t r{}; r < R; ++r) {
          if (r != i) {
            const T factor{a[r][i]};
            a[r] -= factor * a[i];
            aug[r] -= factor * aug[i];
          }
        }
      }
      return aug;
    }
  }
};

// Left-hand scalar multiplication (e.g., 2.0F * mat)
template <typename T, size_t R, size_t C>
  requires std::is_arithmetic_v<T> && (R > 0) && (C > 0)
[[nodiscard]] constexpr Mat<T, R, C> operator*(T scalar, const Mat<T, R, C> &mat) noexcept {
  return mat * scalar;
}

// (1 x R row vector) * (R x C matrix) -> (1 x C row vector)
template <typename T, size_t R, size_t C>
  requires std::is_arithmetic_v<T> && (R > 0) && (C > 0)
[[nodiscard]] constexpr Vec<T, C> operator*(const Vec<T, R> &v, const Mat<T, R, C> &mat) noexcept {
  Vec<T, C> res{};
  for (size_t c{}; c < C; ++c) {
    res[c] = v.dot(mat.col(c));
  }
  return res;
}

using Mat3f = Mat<float, 3, 3>;

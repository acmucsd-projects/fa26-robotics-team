#pragma once
#include <cassert>
#include <cstddef>

// Fixed-size array
template <typename T, size_t N> class Array {
  // NOLINTNEXTLINE
  T storage_[N];

public:
  constexpr Array() noexcept
    requires(requires { T{}; })
  = default;

  template <typename... Ts>
    requires(sizeof...(Ts) == N) && (requires(Ts &&v) { T{static_cast<Ts &&>(v)}; } && ...)
  constexpr Array(Ts &&...vals) : storage_{T{static_cast<Ts &&>(vals)}...} {}

  [[nodiscard]] constexpr size_t size() const noexcept { return N; }

  [[nodiscard]] constexpr bool empty() const noexcept { return N == 0; }

  constexpr T &operator[](size_t idx) noexcept {
    assert(idx < N && "Index access out of range.");
    return storage_[idx];
  }

  constexpr const T &operator[](size_t idx) const noexcept {
    assert(idx < N && "Index access out of range.");
    return storage_[idx];
  }

  constexpr T &front() noexcept
    requires(N > 0)
  {
    return storage_[0];
  }

  constexpr const T &front() const noexcept
    requires(N > 0)
  {
    return storage_[0];
  }

  constexpr T &back() noexcept
    requires(N > 0)
  {
    return storage_[N - 1];
  }

  constexpr const T &back() const noexcept
    requires(N > 0)
  {
    return storage_[N - 1];
  }

  constexpr T *data() noexcept { return storage_; }

  constexpr const T *data() const noexcept { return storage_; }

  constexpr T *begin() noexcept { return storage_; }

  constexpr const T *begin() const noexcept { return storage_; }

  constexpr const T *cbegin() const noexcept { return storage_; }

  constexpr T *end() noexcept { return storage_ + N; }

  constexpr const T *end() const noexcept { return storage_ + N; }

  constexpr const T *cend() const noexcept { return storage_ + N; }
};

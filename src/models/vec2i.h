#pragma once

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace mortido::models {
class vec2i {
 public:
  int x;
  int y;

  vec2i(int x_, int y_) : x(x_), y(y_) {}
  vec2i() : x(0), y(0) {}

  inline vec2i &set(const vec2i &other) {
    *this = other;
    return *this;
  }

  inline vec2i &add(const vec2i &other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  inline vec2i &sub(const vec2i &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  inline vec2i &mul(int scalar) {
    this->x *= scalar;
    this->y *= scalar;
    return *this;
  }

  inline vec2i &div(int scalar) {
    this->x /= scalar;
    this->y /= scalar;
    return *this;
  }

  inline vec2i &operator+=(const vec2i &other) { return add(other); }

  inline vec2i &operator-=(const vec2i &other) { return sub(other); }

  inline vec2i &operator*=(int scalar) { return mul(scalar); }

  inline vec2i &operator/=(int scalar) { return div(scalar); }

  inline int sq_length() const { return this->x * this->x + this->y * this->y; }

  inline double length() const { return std::hypot(this->x, this->y); }

  inline vec2i &abs() {
    this->x = std::abs(this->x);
    this->y = std::abs(this->y);
    return *this;
  }

  /**
   * Rotates vector 90degrees clockwise.
   */
  inline vec2i &rotate90cw() {
    std::swap(x, y);
    y = -y;
    return *this;
  }

  inline vec2i rotated90cw() const { return vec2i{y, -x}; }

  /**
   * Rotates vector 90degrees counter clockwise.
   */
  inline vec2i &rotate90ccw() {
    std::swap(x, y);
    x = -x;
    return *this;
  }

  inline vec2i rotated90ccw() const { return vec2i{-y, x}; }

  bool operator<(const vec2i &other) const {
    if (x < other.x) return true;
    if (x > other.x) return false;
    return y < other.y;
  }

  bool operator==(const vec2i &other) const { return x == other.x && y == other.y; }

  bool operator!=(const vec2i &other) const { return !(*this == other); }

  [[nodiscard]] std::string to_string(int precision = 3) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision);
    out << "(" << x << "; " << y << ")";
    return out.str();
  }
};

inline vec2i operator+(vec2i a, const vec2i &b) {
  return a.add(b);
}

inline vec2i operator-(vec2i a, const vec2i &b) {
  return a.sub(b);
}

inline vec2i operator*(vec2i a, int scalar) {
  return a.mul(scalar);
}

inline vec2i operator*(int scalar, vec2i a) {
  return a.mul(scalar);
}

inline vec2i operator/(vec2i a, int scalar) {
  return a /= scalar;
}

}  // namespace mortido::models

// Specialize std::hash for vec2i
namespace std {
template <>
struct hash<mortido::models::vec2i> {
  std::size_t operator()(const mortido::models::vec2i &v) const noexcept {
    std::size_t h1 = std::hash<int>{}(v.x);
    std::size_t h2 = std::hash<int>{}(v.y);
    return h1 ^ (h2 << 1);  // Combine the hash values
  }
};
}  // namespace std
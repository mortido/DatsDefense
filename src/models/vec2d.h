#pragma once
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include "models/vec2i.h"

namespace mortido::models {

class vec2d {
 public:
  double x;
  double y;

  vec2d(double x_, double y_) : x(x_), y(y_) {}
  vec2d() : x(0), y(0) {}

  inline vec2d &set(const vec2d &other) {
    *this = other;
    return *this;
  }

  inline vec2d &add(const vec2d &other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  inline vec2d &sub(const vec2d &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  inline vec2d &mul(double scalar) {
    this->x *= scalar;
    this->y *= scalar;
    return *this;
  }

  inline vec2d &div(double scalar) {
    this->x /= scalar;
    this->y /= scalar;
    return *this;
  }

  inline vec2d &operator+=(const vec2d &other) { return add(other); }

  inline vec2d &operator-=(const vec2d &other) { return sub(other); }

  inline vec2d &operator*=(double scalar) { return mul(scalar); }

  inline vec2d &operator/=(double scalar) { return div(scalar); }

  inline double sq_length() const { return this->x * this->x + this->y * this->y; }

  inline double length() const { return std::hypot(this->x, this->y); }

  inline vec2d &abs() {
    this->x = std::abs(this->x);
    this->y = std::abs(this->y);
    return *this;
  }

  /**
   * Rotates vector 90 degrees clockwise.
   */
  inline vec2d &rotate90cw() {
    std::swap(x, y);
    y = -y;
    return *this;
  }

  inline vec2d rotated90cw() const { return vec2d{y, -x}; }

  /**
   * Rotates vector 90 degrees counter clockwise.
   */
  inline vec2d &rotate90ccw() {
    std::swap(x, y);
    x = -x;
    return *this;
  }

  inline vec2d rotated90ccw() const { return vec2d{-y, x}; }

  bool operator<(const vec2d &other) const {
    if (x < other.x) return true;
    if (x > other.x) return false;
    return y < other.y;
  }

  bool operator==(const vec2d &other) const { return x == other.x && y == other.y; }

  bool operator!=(const vec2d &other) const { return !(*this == other); }

  [[nodiscard]] std::string to_string(int precision = 3) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision);
    out << "(" << x << "; " << y << ")";
    return out.str();
  }
};

inline vec2d operator+(vec2d a, const vec2d &b) {
  return a.add(b);
}

inline vec2d operator-(vec2d a, const vec2d &b) {
  return a.sub(b);
}

inline vec2d operator*(vec2d a, double scalar) {
  return a.mul(scalar);
}

inline vec2d operator*(double scalar, vec2d a) {
  return a.mul(scalar);
}

inline vec2d operator/(vec2d a, double scalar) {
  return a /= scalar;
}

// Conversion from vec2i to vec2d
inline vec2d to_vec2d(const vec2i &vi) {
  return vec2d(static_cast<double>(vi.x), static_cast<double>(vi.y));
}

}  // namespace mortido::models
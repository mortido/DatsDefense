#pragma once

#include <rapidjson/document.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "models/vec2i.h"

namespace mortido::models {
struct Round {
  int duration;
  std::chrono::system_clock::time_point end_at;
  std::chrono::system_clock::time_point now;
  std::string name;
  std::chrono::system_clock::time_point start_at;
  std::string status;
};

struct RoundList {
  std::string game_name;
  std::chrono::system_clock::time_point now;
  std::vector<Round> rounds;
};

struct Error {
  int err_code;
  std::string message;
};

struct ParticipateResponse {
  int starts_in_sec;
};


}  // namespace mortido::models
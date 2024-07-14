#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "rapidjson/document.h"

namespace mortido::api {

struct Round {
  int duration;
  std::chrono::system_clock::time_point end_at;
  std::chrono::system_clock::time_point now;
  std::string name;
  std::chrono::system_clock::time_point start_at;
  std::string status;

  static Round from_json(const rapidjson::Value& value);
};

struct RoundList {
  std::string game_name;
  std::chrono::system_clock::time_point now;
  std::vector<Round> rounds;

  static RoundList from_json(const rapidjson::Value& value);
};

struct Error {
  int err_code = -1;
  std::string message;

  static std::optional<Error> from_json(const rapidjson::Value& value);
};

struct ParticipateResponse {
  bool registered = false;
  int starts_in_sec = -1;

  static ParticipateResponse from_json(const rapidjson::Value& value);
};

struct CommandResponse {
  static CommandResponse from_json(const rapidjson::Value& value) { return {}; }
};

}  // namespace mortido::models

#pragma once

#include <rapidjson/document.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "models/responses.h"

namespace mortido::models {
inline std::chrono::system_clock::time_point parse_system_time(const std::string& time_str) {
  std::tm tm = {};
  std::istringstream ss(time_str);
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (ss.fail()) {
    throw std::runtime_error("Failed to parse time: " + time_str);
  }
  auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  return time_point;
}

inline Round parse_round(const rapidjson::Value& value) {
  Round round;
  round.duration = value["duration"].GetInt();
  round.end_at = parse_system_time(value["endAt"].GetString());
  round.name = value["name"].GetString();
  round.start_at = parse_system_time(value["startAt"].GetString());
  round.status = value["status"].GetString();
  return round;
}

inline RoundList parse_round_list(const rapidjson::Document& doc) {
  RoundList round_list;
  round_list.game_name = doc["gameName"].GetString();
  round_list.now = parse_system_time(doc["now"].GetString());
  if (doc.HasMember("rounds") && doc["rounds"].IsArray()) {
    for (const auto& round_val : doc["rounds"].GetArray()) {
      Round round = parse_round(round_val);
      round_list.rounds.push_back(round);
    }
  }
  return round_list;
}

inline std::optional<ParticipateResponse> parse_participate_response(const rapidjson::Value& value) {
  if (value.HasMember("startsInSec")){
    return ParticipateResponse{.starts_in_sec = value["startsInSec"].GetInt()};
  }
  return std::nullopt;
}

inline std::optional<Error> parse_error(const rapidjson::Value& value) {
  if (!value.IsObject()) {
    return std::nullopt;
  }

  Error error;
  if (value.HasMember("errCode") && value["errCode"].IsInt()) {
    error.err_code = value["errCode"].GetInt();
  } else {
    return std::nullopt;
  }

  if (value.HasMember("error") && value["error"].IsString()) {
    error.message = value["error"].GetString();
  } else {
    return std::nullopt;
  }

  return error;
}

}  // namespace mortido::models
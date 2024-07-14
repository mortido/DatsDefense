#include "responses.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

std::chrono::system_clock::time_point parse_system_time(const std::string& time_str) {
  std::tm tm = {};
  std::istringstream ss(time_str);
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (ss.fail()) {
    throw std::runtime_error("Failed to parse time: " + time_str);
  }
  auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  return time_point;
}

}  // namespace

namespace mortido::api {

Round Round::from_json(const rapidjson::Value& value) {
  Round round;
  round.duration = value["duration"].GetInt();
  round.end_at = parse_system_time(value["endAt"].GetString());
  round.name = value["name"].GetString();
  round.start_at = parse_system_time(value["startAt"].GetString());
  round.status = value["status"].GetString();
  return round;
}

RoundList RoundList::from_json(const rapidjson::Value& value) {
  RoundList round_list;
  round_list.game_name = value["gameName"].GetString();
  round_list.now = parse_system_time(value["now"].GetString());
  if (value.HasMember("rounds") && value["rounds"].IsArray()) {
    for (const auto& round_val : value["rounds"].GetArray()) {
      auto& round = round_list.rounds.emplace_back(Round::from_json(round_val));
      round.now = round_list.now;
    }
  }
  return round_list;
}

std::optional<Error> Error::from_json(const rapidjson::Value& value) {
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

ParticipateResponse ParticipateResponse::from_json(const rapidjson::Value& value) {
  ParticipateResponse response;
  if (value.HasMember("startsInSec")) {
    // Registered in lobby
    response.starts_in_sec = value["startsInSec"].GetInt();
    response.registered = true;
  } else if (value.HasMember("error") && value["error"].IsString()) {
    std::string err_msg = value["error"].GetString();
    if (err_msg.find("you are participating in this realm") != std::string::npos) {
      response.registered = true;
    }
  }
  return response;
}

}  // namespace mortido::models

#pragma once

#include <rapidjson/document.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "api/api.h"

namespace mortido::api {

class DumpApi : public Api {
 private:
  int turn_ = 0;
  std::string last_world_json_string_;
  std::string last_units_json_string_;
  std::filesystem::path dump_file_;
  std::ifstream file_stream_;
  bool game_ended_ = false;
  std::string name_;

 public:
  explicit DumpApi(std::filesystem::path dump_file)
      : dump_file_(std::move(dump_file)), name_("dump_of_") {
    file_stream_.open(dump_file_, std::ios::in);
    if (!file_stream_.is_open()) {
      throw std::runtime_error("Could not open dump file");
    }
    name_ += dump_file_.filename().stem();
  }

  ~DumpApi() {
    if (file_stream_.is_open()) {
      file_stream_.close();
    }
  }

  bool active() override { return !game_ended_; }

  Round get_current_round(const std::string& prev_round) override {
    auto now = std::chrono::system_clock::now();
    //    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    //
    //    std::ostringstream oss;
    //    oss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d_%H-%M-%S");

    if (prev_round == name_) {
      std::exit(EXIT_SUCCESS);
    }

    // Simulate fetching the current round details (this should be replaced with actual logic)
    Round round;
    round.duration = 1000;
    round.end_at = now + std::chrono::seconds(1000);
    round.now = now;
    round.name = name_;
    round.start_at = now;
    round.status = "active";

    return round;
  }

  ParticipateResponse participate() override {
    return ParticipateResponse{
        .registered = true,
        .starts_in_sec = 0,
    };
  }

  CommandResponse send_command(const Command& command) override {
    turn_++;
    return {};
  }

  rapidjson::Document get_world() override {
    read_turn();
    rapidjson::Document world_doc;
    world_doc.Parse(last_world_json_string_.c_str());
    return world_doc;
  }

  rapidjson::Document get_units() override {
    read_turn();
    rapidjson::Document units_doc;
    units_doc.Parse(last_units_json_string_.c_str());
    if (!units_doc.HasParseError() && units_doc.IsObject()) {
      units_doc["turnEndsInMs"] = 1;
      if (game_ended_) {
        units_doc["gameEndedAt"] = "seychas";
        units_doc["turn"] = turn_;
      }
    }

    return units_doc;
  }

 private:
  void read_turn() {
    if (!file_stream_.is_open()) {
      file_stream_.open(dump_file_, std::ios::in);
      if (!file_stream_.is_open()) {
        throw std::runtime_error("Could not open dump file: " + dump_file_.string());
      }
    }

    std::string line;
    std::string handle;
    std::string temp;

    while (true) {
      // Parse the last known units JSON string to check the turn number
      rapidjson::Document temp_units_doc;
      temp_units_doc.Parse(last_units_json_string_.c_str());
      if (!temp_units_doc.HasParseError() && temp_units_doc.HasMember("turn") &&
          temp_units_doc["turn"].GetInt() >= turn_) {
        turn_ = temp_units_doc["turn"].GetInt();
        break;
      }

      if (!read_dump_segment("REQUEST", line)) {
        game_ended_ = true;
        break;
      }

      std::istringstream stream(line);
      stream >> temp >> temp >> handle;
      std::string* segment_json_ptr = nullptr;
      if (handle == "/play/zombidef/world") {
        segment_json_ptr = &last_world_json_string_;
      } else if (handle == "/play/zombidef/units") {
        segment_json_ptr = &last_units_json_string_;
      }

      if (!read_dump_segment("RESPONSE", line)) {
        game_ended_ = true;
        break;
      }

      if (!read_dump_segment("~~~~~~~~~~~~~", line, segment_json_ptr)) {
        game_ended_ = true;
        break;
      }
    }
  }

  bool read_dump_segment(const std::string& line_flag, std::string& line,
                         std::string* segment = nullptr) {
    line.clear();
    std::stringstream segment_data;
    while (std::getline(file_stream_, line) && line.compare(0, line_flag.size(), line_flag) != 0) {
      segment_data << line << std::endl;
    }
    if (line.compare(0, line_flag.size(), line_flag) != 0) {
      return false;
    }
    if (segment != nullptr) {
      *segment = segment_data.str();
    }
    return true;
  }
};

}  // namespace mortido::api

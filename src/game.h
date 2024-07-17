#pragma once

#include <rapidjson/writer.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

#include "api/api.h"
#include "logger.h"
#include "models/state.h"

using namespace std::chrono_literals;

namespace mortido::game {

class Game {
 public:
  explicit Game(std::string game_id, api::Api& api, bool write_dump = false)
      : id_{std::move(game_id)}, api_(api), log_file_{"data/game_" + id_ + ".log"} {
    loguru::add_file(log_file_.c_str(), loguru::Append, loguru::Verbosity_MAX);
    if (write_dump) {
      dump_file_.open("data/game_" + id_ + ".dump", std::ios::app);
      if (!dump_file_.is_open()) {
        throw std::runtime_error("Could not open dump file");
      }
    }
  }

  ~Game() {
    if (dump_file_.is_open()) {
      dump_file_.close();
    }
    loguru::remove_callback(log_file_.c_str());
    loguru::flush();
  }

  bool run() {
    auto participate_result = api_.participate();
    if (!participate_result.registered) {
      LOG_WARN("NOT registered in game %s lobby", id_.c_str());
      return true;
    }

    LOG_INFO("Registration to game %s successful", id_.c_str());

    if (participate_result.starts_in_sec > 0) {
      LOG_INFO("Wait game to start in %d seconds...", participate_result.starts_in_sec);
      std::this_thread::sleep_for(std::chrono::seconds(participate_result.starts_in_sec) + 250ms);
    }

//    try {
      game_loop();
//    } catch (const api::ApiError& e) {
//      LOG_FATAL("API Exception occurred in game loop: %s", e.what());
//      throw e;
//    } catch (const std::exception& e) {
//      LOG_FATAL("Exception occurred in game loop: %s", e.what());
//      return false;
//    }
    return true;
  }

 private:
  std::string id_;
  api::Api& api_;
  std::filesystem::path log_file_;
  std::ofstream dump_file_;

  std::string team_name_;
  models::State state_;

  bool load_world();
  bool load_units();
  void game_loop();

  void dump_json(const std::string& type, const rapidjson::Document& doc) {
    if (dump_file_.is_open()) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      std::string json_str = buffer.GetString();
      dump_file_ << type << ": " << json_str << std::endl;
      dump_file_.flush();
    }
  }
};

}  // namespace mortido::game

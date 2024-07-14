#pragma once

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
  explicit Game(std::string game_id, api::Api& api)
      : id_{std::move(game_id)}
      , api_(api)
      , log_file_{"data/game_" + id_ + ".log"}
      , dump_file_{"data/game_" + id_ + ".dump", std::ios::app} {
    loguru::add_file(log_file_.c_str(), loguru::Append, loguru::Verbosity_MAX);
  }

  ~Game() {
    dump_file_.close();
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

    try {
      game_loop();
    } catch (const api::ApiError& e) {
      LOG_FATAL("API Exception occurred in game loop: %s", e.what());
      throw e;
    } catch (const std::exception& e) {
      LOG_FATAL("Exception occurred in game loop: %s", e.what());
      return false;
    }
    return true;
  }

 private:
  std::string id_;
  api::Api& api_;
  std::filesystem::path log_file_;
  std::ofstream dump_file_;

  std::string team_name_;
  models::State state_;

  void load();
  void game_loop();
};

}  // namespace mortido::game

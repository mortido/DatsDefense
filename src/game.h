#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "api.h"
#include "logger.h"
#include "models/state.h"

namespace mortido::game {

class Game {
 public:
  explicit Game(std::string game_id)
      : id_{std::move(game_id)}
      , log_file_{"data/game_" + id_ + ".log"}
      , dump_file_{"data/game_" + id_ + ".dump", std::ios::app} {
    loguru::add_file(log_file_.c_str(), loguru::Append, loguru::Verbosity_MAX);
  }

  ~Game() {
    dump_file_.close();
    loguru::remove_callback(log_file_.c_str());
    loguru::flush();
  }

  bool run(net::Api &api) {
    if (!lobby(api)){
      return true;
    }

    try {
      game_loop(api);
    } catch (const net::ApiError &e) {
      LOG_FATAL("API Exception occurred in game loop: %s", e.what());
      throw e;
    } catch (const std::exception &e) {
      LOG_FATAL("Exception occurred in game loop: %s", e.what());
      return false;
    }
    return true;
  }

 private:
  std::string id_;
  std::filesystem::path log_file_;
  std::ofstream dump_file_;

  std::string team_name_;
  models::State state_;

  bool lobby(net::Api &api);
  void load(net::Api &api);
  void game_loop(net::Api &api);
};

}  // namespace mortido::game

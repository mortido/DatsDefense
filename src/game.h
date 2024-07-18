#pragma once

#include <rapidjson/writer.h>

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
  explicit Game(std::string game_id, api::Api& api) : id_{std::move(game_id)}, api_(api) {}

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

  std::string team_name_;
  models::State state_;

  bool load_world() {
    rapidjson::Document world;
    std::optional<api::Error> maybe_error;
    do {
      world = api_.get_world();
      maybe_error = api::Error::from_json(world);
    } while (maybe_error && maybe_error->message.find("lobby ends in") != std::string::npos);

    if (maybe_error) {
      LOG_ERROR("Get world error [%d]: %s", maybe_error->err_code, maybe_error->message.c_str());
      return false;
    }

    state_.init_from_json(world);
    return true;
  }

  bool load_units() {
    rapidjson::Document units;
    std::optional<api::Error> maybe_error;
    do {
      units = api_.get_units();
      maybe_error = api::Error::from_json(units);
    } while (maybe_error && maybe_error->message.find("lobby ends in") != std::string::npos);

    if (maybe_error) {
      LOG_ERROR("Get state error [%d]: %s", maybe_error->err_code, maybe_error->message.c_str());
      return false;
    }

    state_.update_from_json(units);
    return true;
  }

  void game_loop() {
#ifdef DRAW
    rewind_viewer::RewindClient rc("127.0.0.1", 9111);
#endif

    LOG_INFO("Game %s started, team: %s", id_.c_str(), team_name_.c_str());
    load_world();
    while (!state_.game_ended_at) {
      if (!load_units()) {
        return;
      }

      if (state_.game_ended_at) {
        LOG_INFO("GAME %s ENDED AT %s, status: %s", id_.c_str(), state_.game_ended_at->c_str(),
                 state_.end_status.c_str());
      } else {
        if (state_.map.view_zone_updated) {
          load_world();
        }
        auto result = api_.send_command(state_.get_action());
        //      auto maybe_error = api::Error::from_json(result);
        //      if (maybe_error) {
        //        LOG_ERROR("Send command error [%d]: %s", maybe_error->err_code,
        //        maybe_error->message.c_str());
        //      }
      }

#ifdef DRAW
      state_.draw(rc);
#endif

      LOG_INFO("Wait turn %d to end...", state_.turn);
      std::this_thread::sleep_until(state_.turn_end_time);
    }
  }
};

}  // namespace mortido::game

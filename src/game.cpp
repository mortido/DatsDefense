#include "game.h"

#include <thread>

namespace mortido::game {

using namespace std::chrono_literals;

bool Game::load_world() {
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
  dump_json("world", world);
  return true;
}

bool Game::load_units() {
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
  dump_json("state", units);
  return true;
}

void Game::game_loop() {
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

}  // namespace mortido::game
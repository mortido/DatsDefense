#include "game.h"

#include <rapidjson/writer.h>

#include <chrono>
#include <cmath>
#include <thread>

namespace mortido::game {

using namespace std::chrono_literals;

void Game::load() {
  LOG_INFO("Game %s loading...", id_.c_str());
  rapidjson::Document world;
  std::optional<api::Error> maybe_error;
  do {
    world = api_.get_world();
    maybe_error = api::Error::from_json(world);
  } while (maybe_error && maybe_error->message.find("lobby ends in") != std::string::npos);

  if (maybe_error) {
    // todo:?
  } else {
    state_.init_from_json(world);
    // Convert the JSON document to a string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    world.Accept(writer);
    std::string json_str = buffer.GetString();

    // Save the world JSON to the dump file
    if (dump_file_.is_open()) {
      dump_file_ << "world: " << json_str << std::endl;
      dump_file_.flush();
    }
  }

  // todo: print game state (statistics overview)
}

void Game::game_loop() {
  load();
#ifdef DRAW
  rewind_viewer::RewindClient rc("127.0.0.1", 9111);
#endif
  LOG_INFO("Game %s started, team: %s", id_.c_str(), team_name_.c_str());
  while (!state_.me.game_ended_at) {
    auto units_response = api_.get_units();

    while (true) {
      auto maybe_error = api::Error::from_json(units_response);
      if (maybe_error) {
        if (maybe_error->err_code == 28) {
          LOG_ERROR("Missed registration for game %s", id_.c_str());
          return;
        }
        // todo: handle???
        return;
      }
      if (state_.update_from_json(units_response)) {
        break;
      } else {
        units_response = api_.get_units();
      }
    }

    if (state_.me.game_ended_at) {
      LOG_INFO("GAME %s ENDED AT %s", id_.c_str(), state_.me.game_ended_at->c_str());
    } else {
      if (state_.map.view_zone_updated) {
        LOG_INFO("Reload game for updated view");
        load();
      }
      auto result = api_.send_command(state_.get_action());
      auto maybe_error = api::Error::from_json(units_response);
      if (maybe_error) {
        // todo: :(
        return;
      }
#ifdef DRAW
      state_.draw(rc);
#endif
    }



    // Save the state JSON to the dump file
    if (dump_file_.is_open()) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      units_response.Accept(writer);
      std::string json_str = buffer.GetString();

      dump_file_ << "state: " << json_str << std::endl;
      dump_file_.flush();
    }

    LOG_INFO("Wait turn to end...");
    std::this_thread::sleep_until(state_.turn_end_time);
  }
}

}  // namespace mortido::game
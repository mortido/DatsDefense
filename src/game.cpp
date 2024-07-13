#include "game.h"

#include <rapidjson/writer.h>

#include <chrono>
#include <cmath>
#include <thread>

#include "models/json_utils.h"

namespace mortido::game {

using namespace std::chrono_literals;

bool Game::load(net::Api &api) {
  LOG_INFO("Games %s lobby", id_.c_str());
  auto participate_response = api.participate();
  auto maybe_lobby = models::parse_participate_response(participate_response);
  auto maybe_error = models::parse_error(participate_response);
  if (maybe_lobby) {
    LOG_INFO("Waiting to start for %d seconds...", maybe_lobby->starts_in_sec);
    std::this_thread::sleep_for(std::chrono::seconds(maybe_lobby->starts_in_sec));
  } else if (maybe_error) {
    if (maybe_error->message.find("you are participating in this realm") == std::string::npos) {
      return false;
    }
  } else {
    throw std::runtime_error("Not error and not lobby");
  }

  LOG_INFO("Game %s loading...", id_.c_str());
  rapidjson::Document world;
  do {
    world = api.get_world();
    maybe_error = models::parse_error(world);
  } while (maybe_error && maybe_error->message.find("lobby ends in") != std::string::npos);

  if (maybe_error) {
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

  return true;
}

void Game::game_loop(net::Api &api) {
#ifdef DRAW
  rewind_viewer::RewindClient rc("127.0.0.1", 9111);
#endif
  LOG_INFO("Game %s started, team: %s", id_.c_str(), team_name_.c_str());
  while (!state_.me.game_ended_at) {
    auto units_response = api.get_units();

    do {
      auto maybe_error = models::parse_error(units_response);
      if (maybe_error) {
        if (maybe_error->err_code == 28) {
          LOG_ERROR("Missed registration for game %s", id_.c_str());
          return;
        }
        // todo: handle???
        return;
      }
    } while (!state_.update_from_json(units_response));

#ifdef DRAW
    state_.draw(rc);
#endif

    if (state_.me.game_ended_at) {
      LOG_INFO("GAME %s ENDED AT %s", id_.c_str(), state_.me.game_ended_at->c_str());
    } else {
      auto result = api.send_command(state_.get_action());
      auto maybe_error = models::parse_error(units_response);
      if (maybe_error) {
        // todo: :(
        return;
      }
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    units_response.Accept(writer);
    std::string json_str = buffer.GetString();

    // Save the state JSON to the dump file
    if (dump_file_.is_open()) {
      dump_file_ << "state: " << json_str << std::endl;
      dump_file_.flush();
    }
  }
}

}  // namespace mortido::game
#include "game.h"

#include <chrono>
#include <cmath>
#include <thread>

namespace mortido::game {

using namespace std::chrono_literals;

void Game::load(net::Api &api) {
  LOG_INFO("Game %s loading...", id_.c_str());
  //  auto json_state = api_.universe().value();
  //  team_name_ = json_state["name"].GetString();
  //  state_ = models::GameState::from_universe_json(json_state);
  //  state_.map.planets[home_].was_cleared = true;
  //  state_.map.planets[home_].can_have_items = false;
  //  state_.map.planets[garbage_collector_].was_cleared = true;
  //  state_.map.planets[garbage_collector_].can_have_items = false;
  //  LOG_INFO("Game loaded, team: %s", team_name_.c_str());
  //  LOG_INFO("Ship at '%s' fuel %d",
  //           state_.ship.planet.c_str(),
  //           state_.ship.fuelUsed);
  //
  //  min_load_ = static_cast<int>(std::ceil(static_cast<double>(state_.ship.cargo->capacity()) *
  //  0.3)); min_load_on_planet_ =
  //  static_cast<int>(std::ceil(static_cast<double>(state_.ship.cargo->capacity()) * 0.05));
  //  LOG_INFO("Capacity: %d; Min load: %d; Min load on planet: %d",
  //           state_.ship.cargo->capacity(),
  //           min_load_,
  //           min_load_on_planet_);
  //  LOG_INFO("Planets in universe %zu", state_.map.planets.size());

  // todo: print game state (statistics overview)

  // todo: save to dump file.
  dump_file_.flush();
}

void Game::game_loop(net::Api &api) {
  LOG_INFO("Game %s started, team: %s", id_.c_str(), team_name_.c_str());
}

}  // namespace mortido::game
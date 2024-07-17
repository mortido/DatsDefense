#include "state.h"

namespace mortido::models {

bool State::update_from_json(const rapidjson::Document& doc) {
  int next_turn = doc["turn"].GetInt();
  if (turn == next_turn) {
    return false;
  }

  if(next_turn - turn > 1) {
    LOG_ERROR("TURN SKIPPED %d -> %d", turn, next_turn);
  }

  int turn_ends_in_ms = doc["turnEndsInMs"].GetInt();
  turn_end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(turn_ends_in_ms);
  turn = next_turn;
  const auto& player_json = doc["player"];
  me.update_from_json(player_json);
  if (player_json.HasMember("gameEndedAt") && !player_json["gameEndedAt"].IsNull()) {
    game_ended_at = player_json["gameEndedAt"].GetString();
    end_status = "GAME OVER";
  } else {
    game_ended_at.reset();
  }

  map.clear();
  if (doc.HasMember("base") && doc["base"].IsArray()) {
    for (const auto& block_val : doc["base"].GetArray()) {
      Building b;
      b.update_from_json(block_val);
      b.is_enemy = false;
      map.add_building(b);
    }
  }

  if (doc.HasMember("enemyBlocks") && doc["enemyBlocks"].IsArray()) {
    for (const auto& block_val : doc["enemyBlocks"].GetArray()) {
      Building b;
      b.update_from_json(block_val);
      b.is_enemy = true;
      map.add_building(b);
    }
  }


  if (doc.HasMember("zombies") && doc["zombies"].IsArray()) {
    for (const auto& zombie_val : doc["zombies"].GetArray()) {
      Zombie zombie;
      zombie.update_from_json(zombie_val);
      map.add_zombie(std::move(zombie));
    }
  }

  map.update(turn);
  return true;
}

void State::init_from_json(const rapidjson::Document& doc) {
  if (doc.HasMember("zpots") && doc["zpots"].IsArray()) {
    map.clear_spawns_and_walls();
    for (const auto& zp : doc["zpots"].GetArray()) {
      vec2i spot(zp["x"].GetInt(), zp["y"].GetInt());
      std::string type = zp["type"].GetString();
      if (type == "default") {
        map.add_spawn(spot);
      } else if (type == "wall") {
        map.add_wall(spot);
      } else {
        throw std::runtime_error("UNKNOWN SPOT TYPE");
      }
    }
  }
}
}  // namespace mortido::models

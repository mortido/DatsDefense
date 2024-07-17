#pragma once
#include <rapidjson/document.h>

#include <optional>
#include <vector>

#include "logger.h"
#include "models/building.h"

namespace mortido::models {
struct Player {
  std::string name;
  int gold;
  int enemy_block_kills;
  int points;
  int zombie_kills;


  void update_from_json(const rapidjson::Value& value) {
    name = value["name"].GetString();
    gold = value["gold"].GetInt();
    enemy_block_kills = value["enemyBlockKills"].GetInt();
    points = value["points"].GetInt();
    zombie_kills = value["zombieKills"].GetInt();
  }

};
}
#pragma once

#include <rapidjson/document.h>

#include <vector>

#include "logger.h"
#include "models/map.h"
#include "models/player.h"
#include "models/requests.h"
#include "models/vec2i.h"
#include "models/zombie.h"

#ifdef DRAW
#include <rewind_viewer/RewindClient.h>
#include <rewind_viewer/colors.h>

#include "models/vec2d.h"
#endif

namespace mortido::models {

struct State {
  int turn = -1;
  Player me;
  Map map;
  std::chrono::steady_clock::time_point turn_end_time;

  bool update_from_json(const rapidjson::Document& doc);

  void init_from_json(const rapidjson::Document& doc);

  Command get_action() {
    models::Command cmd{
        .attack = attack(),
        .build = build(),
        .move_base = move_base(),
    };
  }

  std::vector<AttackCommand> attack() {
    std::vector<AttackCommand> result;
    for (size_t i : map.my_active_buildings) {
      const auto& b = map.buildings[i];
      double best_score = -1.0;
      vec2i target;
      for (const auto& attack_dir : map.get_attack_dirs(b.range)) {
        vec2i position = b.position + attack_dir;
        if (map.on_map(position)) {
          double score = map.get_attack_score(position, b.attack);
          if (score > best_score) {
            best_score = score;
            target = position;
          }
        }
      }
      if (best_score > 0.0) {
        result.emplace_back(AttackCommand{.block_id = b.id, .target = target});
        map.attack(target, b.attack);
      }
    }
    return result;
  }

  std::vector<vec2i> build() {
    std::vector<vec2i> result;
    std::sort(map.build_candidates.begin(), map.build_candidates.end(),
              [&](const vec2i& a, const vec2i& b) -> bool {
                // todo: speed up?
                return map.at(a).danger_score < map.at(b).danger_score;
              });
    for (const auto& cand : map.build_candidates) {
      if (me.gold > 0) {
        result.push_back(cand);
        me.gold--;
        // todo: add building for "move_base"
        // todo: recalculate candidates?
      }
    }
    return result;
  }

  std::optional<vec2i> move_base() {
    vec2i new_pos = map.buildings[map.my_base].position;
    double danger = map.at(new_pos).danger_score;
    for (size_t i : map.my_active_buildings) {
      const auto& p = map.buildings[i].position;
      double d = map.at(p).danger_score;
      if (d < danger) {
        danger = d;
        new_pos = p;
      }
    }

    if (new_pos == map.buildings[map.my_base].position) {
      return std::nullopt;
    } else {
      return new_pos;
    }
  }

#ifdef DRAW
  void draw(rewind_viewer::RewindClient& rc) {
    using namespace rewind_viewer::colors;

    // Define colors
    constexpr uint32_t kWallColor = gray::SlateGray;
    constexpr uint32_t kSpawnColor = yellow::Gold;
    constexpr uint32_t kBuildCandidateColor = green::MediumSpringGreen;

    // Draw the map grid
    if (turn == 0) {
      rc.map(to_vec2d(vec2i{0, 0}), to_vec2d(map.size), map.size.x, map.size.y);
    }

    rc.switch_to_layer(2);
    // Draw walls
    for (const auto& wall : map.walls) {
      rc.rectangle(to_vec2d(wall), vec2d{1.0, 1.0}, kWallColor, true);
    }

    // Draw spawn points
    for (const auto& spawn : map.spawns) {
      rc.rectangle(to_vec2d(spawn), vec2d{1.0, 1.0}, kSpawnColor, true);
    }

    // Draw buildings
    for (size_t i = 0; i < map.buildings.size(); i++) {
      bool is_active = map.my_active_buildings.count(i);
      map.buildings[i].draw(rc, is_active);
    }

    rc.switch_to_layer(3);
    rc.set_opacity(100);
    // Draw build candidates
    for (const auto& candidate : map.build_candidates) {
      rc.rectangle(to_vec2d(candidate) + vec2d{0.3, 0.3}, vec2d{0.4, 0.4}, kBuildCandidateColor,
                   true);
    }
    rc.set_opacity(0xFF);

    // Draw zombies
    for (const auto& zombie : map.zombies) {
      zombie.draw(rc);
    }

    rc.end_frame();
  }

#endif
};

}  // namespace mortido::models

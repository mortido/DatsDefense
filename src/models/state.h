#pragma once

#include <rapidjson/document.h>

#include <vector>
#include <optional>
#include <chrono>

#include "api/requests.h"
#include "logger.h"
#include "models/map.h"
#include "models/player.h"
#include "models/vec2i.h"
#include "models/vec2d.h"
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

  std::vector<vec2i> build_command;
  std::optional<vec2i> move_base_command;
  std::vector<api::AttackCommand> attack_command;

  bool update_from_json(const rapidjson::Document& doc);

  void init_from_json(const rapidjson::Document& doc);

  api::Command get_action() {
    return api::Command{
        .attack = attack(),
        .build = build(),
        .move_base = move_base(),
    };
  }

  const std::vector<api::AttackCommand>& attack() {
    attack_command.clear();
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
        attack_command.emplace_back(
            api::AttackCommand{.block_id = b.id, .target = target, .source = b.position});
        me.gold+=map.attack(target, b.attack);
      }
    }
    return attack_command;
  }

  const std::vector<vec2i>& build() {
    const static std::array<int, 5> shift_pattern = {0, 2, 4, 1, 3};
    build_command.clear();

    struct CandidateScore {
      vec2i position;
      double score;
    };

    vec2i base_pos = map.buildings.at(map.my_base).position;

    std::vector<CandidateScore> candidate_scores;
    for (const auto& cand : map.build_candidates) {
      double danger_score = map.at(cand).danger_score * map.at(cand).danger_multiplier;
      size_t nearest_cluster_size = map.get_nearest_cluster_size(cand);
      //      double distance_to_centroid = (to_vec2d(cand) - centroid).sq_length();
      double distance_to_centroid = (cand - base_pos).length();

      // Calculate distance to the nearest spawn point
      double min_distance_to_spawn = std::numeric_limits<double>::max();
      double min_distance_to_enemy = std::numeric_limits<double>::max();


        for (const auto& spawn : map.spawns) {
          double distance_to_spawn = (cand - spawn).length();
          if (distance_to_spawn < min_distance_to_spawn) {
            min_distance_to_spawn = distance_to_spawn;
          }
        }


      for (const auto enemy : map.enemy_buildings) {
        double distance_to_enemy = (cand - map.buildings.at(enemy).position).length();
        if (distance_to_enemy < min_distance_to_enemy) {
          min_distance_to_enemy = distance_to_enemy;
        }
      }

      double score = danger_score + 0.25 * distance_to_centroid;

      if (turn < 240) {
        score += 0.2 * min_distance_to_spawn;
        score += 0.2 * min_distance_to_enemy;
      } else {
//        min_distance_to_spawn = 0;
        score -= 0.2 * min_distance_to_enemy;
      }

      if (nearest_cluster_size > 10) {
        score *= 1.0 / static_cast<double>(nearest_cluster_size);
      }
      candidate_scores.push_back(CandidateScore{cand, score});
    }

    // Sort candidates by their score (lower score is better)
    std::sort(candidate_scores.begin(), candidate_scores.end(),
              [](const CandidateScore& a, const CandidateScore& b) { return a.score < b.score; });

    for (const auto& candidate : candidate_scores) {
      if (candidate.position.x % 5 == shift_pattern[candidate.position.y % shift_pattern.size()] &&
          turn > 140) {
        continue;
      }
//      if ((me.gold > 0 && (turn > 300 || turn < 50)) || me.gold > turn) {
      if (me.gold > 0) {
        build_command.push_back(candidate.position);
        me.gold--;
      }
    }
    return build_command;
  }

  //  const std::optional<vec2i>& move_base() {
  //    if (map.my_base < 0) {
  //      move_base_command.reset();
  //      return move_base_command;
  //    }
  //
  //    vec2i new_pos = map.buildings.at(map.my_base).position;
  //    double danger = map.at(new_pos).danger_score;
  //    for (size_t i : map.my_active_buildings) {
  //      const auto& p = map.buildings[i].position;
  //      double d = map.at(p).danger_score * map.at(p).danger_multiplier;
  //      if (d < danger) {
  //        danger = d;
  //        new_pos = p;
  //      }
  //    }
  //
  //    if (new_pos == map.buildings.at(map.my_base).position) {
  //      move_base_command.reset();
  //    } else {
  //      move_base_command = new_pos;
  //    }
  //    return move_base_command;
  //  }

  const std::optional<vec2i>& move_base() {
    if (map.my_base < 0) {
      move_base_command.reset();
      return move_base_command;
    }

    // Calculate the centroid of the main cluster
    vec2d centroid{0.0, 0.0};
    double all_health = 0.0;
    size_t main_cluster_size = 0;

    for (size_t i : map.my_active_buildings) {
      if (map.clusters->find(i) == map.clusters->find(map.my_base)) {
        centroid += to_vec2d(map.buildings[i].position) * map.buildings[i].health;
        all_health += map.buildings[i].health;
        main_cluster_size++;
      }
    }
    if (main_cluster_size > 0) {
//      centroid /= static_cast<double>(main_cluster_size);
      centroid /= all_health;
    }

    vec2i new_pos = map.buildings.at(map.my_base).position;
    double best_score = std::numeric_limits<double>::max();
    double distance_to_centroid = (to_vec2d(new_pos) - centroid).length();

    auto calculate_score = [&](const vec2i& pos) {
      double danger = map.at(pos).danger_score * map.at(pos).danger_multiplier;
      double dist_to_centroid = (to_vec2d(pos) - centroid).length();

      // Calculate penalty for being close to spawns
      double min_distance_to_spawn = 1000.0;
      for (const auto& spawn : map.spawns) {
        double distance_to_spawn = (pos - spawn).sq_length();
        if (distance_to_spawn < min_distance_to_spawn) {
          min_distance_to_spawn = distance_to_spawn;
        }
      }

      return 10.0*danger + dist_to_centroid - std::sqrt(min_distance_to_spawn);
    };

    for (size_t i : map.my_active_buildings) {
      const auto& p = map.buildings[i].position;
      double score = calculate_score(p);
      if (score < best_score) {
        best_score = score;
        new_pos = p;
      }
    }

//    for (const auto& p : build_command) {
//      double score = calculate_score(p);
//      if (score < best_score) {
//        best_score = score;
//        new_pos = p;
//      }
//    }

    if (new_pos == map.buildings.at(map.my_base).position) {
      move_base_command.reset();
    } else {
      move_base_command = new_pos;
    }
    return move_base_command;
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

    // Draw base move command
    rc.switch_to_layer(8);
    if (move_base_command) {
      vec2i current_position = map.buildings.at(map.my_base).position;
      vec2d start = to_vec2d(current_position) + vec2d{0.5, 0.5};
      vec2d end = to_vec2d(*move_base_command) + vec2d{0.5, 0.5};
      rc.line(start, end, yellow::Gold);
    }

    // Draw attack commands
    for (const auto& attack : attack_command) {
      vec2d target_pos = to_vec2d(attack.target) + vec2d{0.5, 0.5};
      rc.line(target_pos + vec2d{-0.5, -0.5}, target_pos + vec2d{0.5, 0.5}, red::Crimson);
      rc.line(target_pos + vec2d{-0.5, 0.5}, target_pos + vec2d{0.5, -0.5}, red::Crimson);
      rc.line(target_pos, to_vec2d(attack.source) + vec2d{0.5, 0.5}, red::Crimson);
    }

    // Draw build commands
    for (const auto& build : build_command) {
      vec2d build_pos = to_vec2d(build) + vec2d{0.5, 0.5};
      rc.circle(build_pos, 0.4, green::Green, false);
    }

    rc.switch_to_layer(7);
    std::vector<uint32_t> field_colors;
    vec2i pos{0, 0};
    uint32_t alpha = 100;
    alpha <<=24;
    while (pos.y < map.size.y) {
      pos.x = 0;
      while (pos.x < map.size.x) {
        uint32_t color = 0;
        double danger = map.at(pos).danger_score;
        if (danger > 0.0000001) {
          color = heat_color(danger, 0.0, map.max_danger) | alpha;
        }
        field_colors.push_back(color);
        pos.x++;
      }
      pos.y++;
    }

    rc.tiles(vec2d(0.0, 0.0), vec2d(1.0, 1.0), map.size.x, &field_colors, false);

    rc.log_text("Turn %d", turn);

    rc.end_frame();
  }

#endif
};

}  // namespace mortido::models

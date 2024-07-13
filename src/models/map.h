#pragma once
#include <queue>
#include <unordered_set>
#include <utility>
#include <vector>

#include "models/building.h"
#include "models/vec2i.h"
#include "models/zombie.h"

namespace mortido::models {

struct Cell {
  enum class Type {
    normal,
    wall,
    spawn,
  };

  double danger_score = 0.0;
  Type type = Type::normal;
  std::vector<size_t> zombies;
  int building = -1;
};

class Map {
 private:
  std::vector<std::vector<Cell>> data_;
  std::vector<vec2i> all_directions_;
  std::vector<vec2i> straight_directions_;
  std::vector<vec2i> diagonal_directions_;
  std::unordered_map<int, std::vector<vec2i>> attack_cache_;

 public:
  vec2i size;
  std::vector<Zombie> zombies;
  std::vector<Building> buildings;
  std::vector<size_t> my_buildings;
  std::unordered_set<size_t> my_active_buildings;
  std::vector<vec2i> build_candidates;

  int my_base = -1;
  std::vector<size_t> enemy_buildings;
  std::vector<vec2i> walls;
  std::vector<vec2i> spawns;

  Map() {
    straight_directions_.emplace_back(-1, 0);
    straight_directions_.emplace_back(1, 0);
    straight_directions_.emplace_back(0, -1);
    straight_directions_.emplace_back(0, 1);

    all_directions_.emplace_back(-1, 0);
    all_directions_.emplace_back(-1, -1);
    all_directions_.emplace_back(-1, 1);
    all_directions_.emplace_back(1, 0);
    all_directions_.emplace_back(1, 1);
    all_directions_.emplace_back(1, -1);
    all_directions_.emplace_back(0, -1);
    all_directions_.emplace_back(0, 1);

    diagonal_directions_.emplace_back(-1, -1);
    diagonal_directions_.emplace_back(-1, 1);
    diagonal_directions_.emplace_back(1, 1);
    diagonal_directions_.emplace_back(1, -1);
  }

  void add_wall(vec2i pos) {
    ensure_size(pos);
    walls.emplace_back(pos);
    at(pos).type = Cell::Type::wall;
  }

  void add_spawn(vec2i pos) {
    ensure_size(pos);
    spawns.emplace_back(pos);
    at(pos).type = Cell::Type::spawn;
  }

  void add_building(Building b) {
    ensure_size(b.position);

    at(b.position).building = buildings.size();
    if (b.is_enemy) {
      enemy_buildings.push_back(buildings.size());
    } else {
      my_buildings.push_back(buildings.size());
      if (b.is_head) {
        my_base = buildings.size();
      }
    }

    buildings.emplace_back(std::move(b));
  }

  void add_zombie(Zombie z) {
    ensure_size(z.position);
    at(z.position).zombies.push_back(zombies.size());
    zombies.emplace_back(std::move(z));
  }



  void ensure_size(vec2i pos) {
    if (pos.x >= size.x) {
      data_.resize(pos.x + 1, std::vector<Cell>(size.y));
      size.x = pos.x + 1;
    }
    if (pos.y >= size.y) {
      for (auto& col_vec : data_) {
        col_vec.resize(pos.y + 1);
      }
      size.y = pos.y + 1;
    }
  }

  void clear() {
    for (auto& row : data_) {
      for (auto& cell : row) {
        cell.zombies.clear();
        cell.building = -1;
        cell.danger_score = 0.0;
      }
    }
    my_buildings.clear();
    enemy_buildings.clear();
    zombies.clear();
    buildings.clear();
    my_active_buildings.clear();
    build_candidates.clear();
    my_base = -1;
  }

  [[nodiscard]] inline bool on_map(vec2i pos) const {
    return pos.x >= 0 && pos.y >= 0 && pos.x < size.x && pos.y < size.y;
  }

  void update() {
    std::unordered_set<vec2i> visited;
    for (auto i : my_buildings) {
      auto& b = buildings.at(i);
      visited.insert(b.position);
      for (const auto& dir : straight_directions_) {
        vec2i neighbor = b.position + dir;
        if (on_map(neighbor) && !visited.contains(neighbor)) {
          visited.insert(neighbor);
          if (can_build(neighbor)) {
            build_candidates.emplace_back(neighbor);
          }
        }
      }
    }

    for (const auto& zombie : zombies) {
      auto& cell = at(zombie.position);
//      cell.danger_score += zombie.attack;
    }
    
    for (auto i : enemy_buildings) {
      const auto& building = buildings.at(i);
      for (const auto& tile : get_attack_dirs(building.range)) {
        vec2i attack_pos = building.position + tile;
        if (on_map(attack_pos)) {
          auto& cell = at(attack_pos);
          cell.danger_score += building.attack;
        }
      }
    }

    if (my_base >= 0) {
      std::queue<size_t> to_explore;
      visited.clear();
      to_explore.push(my_base);
      visited.insert(buildings.at(my_base).position);

      while (!to_explore.empty()) {
        auto current = to_explore.front();
        to_explore.pop();
        my_active_buildings.insert(current);

        for (const auto& dir : straight_directions_) {
          vec2i neighbor = buildings.at(current).position + dir;
          if (on_map(neighbor) && !visited.contains(neighbor)) {
            visited.insert(neighbor);
            const Cell& cell = at(neighbor);
            if (cell.building >= 0 && !buildings.at(cell.building).is_enemy) {
              to_explore.push(cell.building);
            }
          }
        }
      }
    }
  }
  
  
  const std::vector<vec2i>& get_attack_dirs(int range) {
    if (attack_cache_.find(range) != attack_cache_.end()) {
      return attack_cache_[range];
    }
    
    std::vector<vec2i> tiles;
    int range_squared = range * range;

    for (int x = -range; x <= range; ++x) {
      for (int y = -range; y <= range; ++y) {
        if (x * x + y * y <= range_squared) {
          tiles.emplace_back(x, y);
        }
      }
    }

    attack_cache_[range] = tiles;
    return attack_cache_[range];
  }

  void attack(const vec2i& pos, int power) {

    auto& cell = at(pos);
    //todo: update attack score in cell
  }

  double get_attack_score(const vec2i& pos, int power){
    auto& cell = at(pos);
    double score = 0.0;
    if (cell.building>=0) {
      score+=1.0;
    }
    for(size_t z_i:cell.zombies){
      const auto& zombie = zombies.at(z_i);
      score+=1.0;
    }

    return score;
  }

  [[nodiscard]] Cell& at(const vec2i& pos) { return data_[pos.x][pos.y]; }
  [[nodiscard]] const Cell& at(const vec2i& pos) const { return data_[pos.x][pos.y]; }

 private:
  [[nodiscard]] inline bool can_build(vec2i pos) const {
    //    if (!on_map(pos)) return false;
    const Cell& cell = at(pos);

    // Cannot build on walls, spawns, or other buildings
    if (cell.type == Cell::Type::wall || cell.type == Cell::Type::spawn || cell.building >= 0) {
      return false;
    }

    // Check adjacent cells
    for (const auto& dir : straight_directions_) {
      vec2i adjacent_pos = pos + dir;
      if (on_map(adjacent_pos)) {
        const Cell& adjacent_cell = at(adjacent_pos);
        // Cannot build adjacent to walls or spawns
        if (adjacent_cell.type == Cell::Type::wall || adjacent_cell.type == Cell::Type::spawn) {
          return false;
        }
        // Cannot build in the vicinity of opponent's base (within 1-cell radius)
        if (adjacent_cell.building >= 0 && buildings.at(adjacent_cell.building).is_enemy) {
          return false;
        }
      }
    }

    for (const auto& dir : diagonal_directions_) {
      vec2i adjacent_pos = pos + dir;
      if (on_map(adjacent_pos)) {
        const Cell& adjacent_cell = at(adjacent_pos);
        // Cannot build in the vicinity of opponent's base (within 1-cell radius)
        if (adjacent_cell.building >= 0 && buildings.at(adjacent_cell.building).is_enemy) {
          return false;
        }
      }
    }

    return true;
  }
};

}  // namespace mortido::models
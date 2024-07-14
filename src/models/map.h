#pragma once
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "models/building.h"
#include "models/vec2i.h"
#include "models/zombie.h"

namespace mortido::models {

constexpr static size_t kLookAhead = 10;
constexpr static double kTimeFactor = 0.9;

struct Cell {
  enum class Type {
    normal,
    wall,
    spawn,
  };

  double danger_score = 0.0;
  double danger_multiplier = 1.0;
  Type type = Type::normal;
  std::vector<size_t> zombies;
  int building = -1;
};

class UnionFind {
 public:
  UnionFind(size_t n) : parent_(n), rank_(n, 0), size_(n, 1) {
    for (size_t i = 0; i < n; ++i) {
      parent_[i] = i;
    }
  }

  size_t find(size_t x) {
    if (parent_[x] != x) {
      parent_[x] = find(parent_[x]);  // Path compression
    }
    return parent_[x];
  }

  size_t cluster_size(size_t x) { return size_[find(x)]; }

  void unite(size_t x, size_t y) {
    size_t rootX = find(x);
    size_t rootY = find(y);
    if (rootX != rootY) {
      if (rank_[rootX] > rank_[rootY]) {
        parent_[rootY] = rootX;
        size_[rootX] += size_[rootY];
      } else if (rank_[rootX] < rank_[rootY]) {
        parent_[rootX] = rootY;
        size_[rootY] += size_[rootX];
      } else {
        parent_[rootY] = rootX;
        size_[rootX] += size_[rootY];
        rank_[rootX]++;
      }
    }
  }

 private:
  std::vector<size_t> parent_;
  std::vector<size_t> rank_;
  std::vector<size_t> size_;
};

class Map {
 private:
  std::vector<std::vector<Cell>> data_;
  std::vector<vec2i> all_directions_;
  std::vector<vec2i> straight_directions_;
  std::vector<vec2i> diagonal_directions_;
  std::unordered_map<int, std::vector<vec2i>> attack_cache_;
  double const_time_factor;
  std::unordered_map<Zombie::Type, Zombie> proto_zombie;

 public:
  vec2i size;
  std::vector<Zombie> zombies;
  std::vector<Building> buildings;
  std::vector<size_t> my_buildings;
  std::unordered_set<size_t> my_active_buildings;
  std::vector<vec2i> build_candidates;

  std::unique_ptr<UnionFind> clusters;

  vec2i view_min;
  vec2i view_max;
  bool view_zone_updated = false;

  int my_base = -1;
  std::vector<size_t> enemy_buildings;
  std::vector<vec2i> walls;
  std::vector<vec2i> spawns;
  double max_danger;

  Map() : const_time_factor(0.0) {
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

    double t = 1.0;
    for (size_t i = 0; i < kLookAhead; i++) {
      const_time_factor += t;
      t *= kTimeFactor;
    }
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

  void clear_spawns_and_walls() {
    walls.clear();
    spawns.clear();
  }

  void clear() {
    for (auto& row : data_) {
      for (auto& cell : row) {
        cell.zombies.clear();
        cell.building = -1;
        cell.danger_score = 0.0;
        cell.danger_multiplier = 1.0;
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

  void update_my_buildings() {
    vec2i temp_pos;
    clusters = std::make_unique<UnionFind>(buildings.size());

    for (auto i : my_buildings) {
      auto& b = buildings.at(i);
      for (const auto& tile : get_attack_dirs(b.range)) {
        temp_pos.set(b.position).add(tile);
        if (on_map(temp_pos)) {
          Cell& attack_cell = at(temp_pos);
          attack_cell.danger_multiplier *= 0.95;
        }
        if (temp_pos.x < view_min.x) {
          view_min.x = temp_pos.x;
          view_zone_updated = true;
        }

        if (temp_pos.y < view_min.y) {
          view_min.y = temp_pos.y;
          view_zone_updated = true;
        }

        if (temp_pos.x > view_max.x) {
          view_max.x = temp_pos.x;
          view_zone_updated = true;
        }

        if (temp_pos.y > view_max.y) {
          view_max.y = temp_pos.y;
          view_zone_updated = true;
        }
      }

      for (const auto& dir : straight_directions_) {
        temp_pos.set(b.position).add(dir);
        if (on_map(temp_pos)) {
          const Cell& neighbor_cell = at(temp_pos);
          if (neighbor_cell.building >= 0) {  // && !buildings[neighbor_cell.building].is_enemy) {
            clusters->unite(i, neighbor_cell.building);
          }
        }
      }
    }

    // TODO: clusters to unite with buildings...
  }

  size_t get_nearest_cluster_size(vec2i base_position) {
    size_t base_cluster = clusters->find(my_base);
    size_t nearest_cluster_size = 0;
    int nearest_distance = std::numeric_limits<int>::max();
    vec2i nearest_building_position;

    for (auto i : my_buildings) {
      size_t cluster = clusters->find(i);
      if (cluster == base_cluster) {
        continue;  // Ignore the cluster where the base is located
      }
      const auto& building = buildings[i];
      int distance = (base_position - building.position).sq_length();
      if (distance < nearest_distance) {
        nearest_distance = distance;
        nearest_cluster_size = clusters->cluster_size(i);
        nearest_building_position = building.position;
      }
    }

    return nearest_cluster_size;
  }

  void update_zombies() {
    for (auto& zombie : zombies) {
      //      auto& cell = at(zombie.position);
      //      cell.danger_score += zombie.attack;
      double t = 1.0;
      zombie.danger = 1.0;
      vec2i temp_pos;

      if (!proto_zombie.contains(zombie.type)) {
        proto_zombie[zombie.type] = zombie;
      } else {
        proto_zombie[zombie.type].update_proto(zombie);
      }

      auto future_positions = zombie.get_future_positions(kLookAhead, kTimeFactor,
                                                          proto_zombie[zombie.type].wait_turns);

      for (size_t fp = 0; fp < future_positions.size(); fp++) {
        const auto& future_pos = future_positions[fp];
        if (on_map(future_pos.pos)) {
          auto& cell = at(future_pos.pos);
          if (cell.type != Cell::Type::normal) {
            break;
          }
          if (cell.building >= 0) {
            cell.danger_score += future_pos.damage;
            if (!buildings[cell.building].is_enemy) {
              zombie.danger += future_pos.damage;
            }

            if (zombie.type == Zombie::Type::bomber) {
              for (const auto& shift : all_directions_) {
                temp_pos.set(future_pos.pos).add(shift);
                if (on_map(temp_pos)) {
                  auto& cell_2 = at(temp_pos);
                  cell_2.danger_score += future_pos.damage;
                  if (cell_2.building >= 0 && !buildings[cell.building].is_enemy) {
                    zombie.danger += future_pos.damage;
                  }
                }
              }
            } else if (zombie.type == Zombie::Type::liner) {
              temp_pos.set(future_pos.pos).add(future_pos.dir);
              while (on_map(temp_pos)) {
                auto& cell_2 = at(temp_pos);
                if (cell_2.building < 0) {
                  break;
                }

                cell_2.danger_score += t * zombie.attack;
                if (cell_2.building >= 0 && !buildings[cell_2.building].is_enemy) {
                  zombie.danger += future_pos.damage;
                }
                temp_pos.add(future_pos.dir);
              }
            }

            if (zombie.type != Zombie::Type::juggernaut &&
                zombie.type != Zombie::Type::chaos_knight &&
                (fp + 1 >= future_positions.size() ||
                 future_positions[fp + 1].step != future_pos.step)) {
              break;
            }
          } else {
            // TODO:??
            cell.danger_score += future_pos.damage;
          }
        } else {
          break;
        }
      }
    }
  }

  void update_enemy_buildings() {
    vec2i temp_pos;
    for (auto i : enemy_buildings) {
      auto& building = buildings.at(i);
      building.danger = 1.0;
      for (const auto& tile : get_attack_dirs(building.range)) {
        temp_pos.set(building.position).add(tile);
        if (on_map(temp_pos)) {
          auto& cell = at(temp_pos);
          cell.danger_score += const_time_factor * building.attack;
          max_danger = std::max(max_danger, cell.danger_score);

          if (cell.building >= 0 && !buildings.at(cell.building).is_enemy) {
            const auto& my_building = buildings.at(i);
            if (my_building.is_head) {
              //              building.danger += const_time_factor * building.attack * 100;
              building.danger += my_building.health * 100;
            } else {
              building.danger += my_building.health;
              //              building.danger += const_time_factor * building.attack;
            }
          }
        }
      }
    }
  }

  void update() {
    view_zone_updated = false;
    vec2i temp_pos;

    update_my_buildings();
    update_enemy_buildings();
    update_zombies();

    if (my_base >= 0) {
      std::unordered_set<vec2i> visited;
      std::queue<size_t> to_explore;
      to_explore.push(my_base);
      visited.insert(buildings.at(my_base).position);

      while (!to_explore.empty()) {
        auto current = to_explore.front();
        to_explore.pop();
        my_active_buildings.insert(current);

        for (const auto& dir : straight_directions_) {
          temp_pos.set(buildings.at(current).position).add(dir);
          if (on_map(temp_pos) && !visited.contains(temp_pos)) {
            visited.insert(temp_pos);
            const Cell& cell = at(temp_pos);
            if (cell.building >= 0 && !buildings.at(cell.building).is_enemy) {
              to_explore.push(cell.building);
            }

            if (can_build(temp_pos)) {
              build_candidates.emplace_back(temp_pos);
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

  int attack(const vec2i& pos, int power) {
    int gold = 0;
    auto& cell = at(pos);
    if (cell.building >= 0 && buildings.at(cell.building).is_enemy) {
      buildings.at(cell.building).damage_taken += power;
    }
    for (size_t z_i : cell.zombies) {
      auto& zombie = zombies.at(z_i);
      if (zombie.health > zombie.damage_taken) {
        zombie.damage_taken += power;
        if (zombie.health <= zombie.damage_taken) {
          gold++;
        }
      }
    }
    return gold;
  }

  double get_attack_score(const vec2i& pos, int power) {
    auto& cell = at(pos);
    double score = 0.0;
    if (cell.building >= 0 && buildings.at(cell.building).is_enemy) {
      const auto& building = buildings.at(cell.building);
      if (building.health > building.damage_taken) {
        int strikes = (building.health + power - 1) / power;
        if (building.is_head) {
          score += static_cast<double>(strikes) * building.danger * 100.0;
        } else {
          score += static_cast<double>(strikes) * building.danger;
        }
      }
    }
    for (size_t z_i : cell.zombies) {
      const auto& zombie = zombies.at(z_i);
      if (zombie.health > zombie.damage_taken) {
        int strikes = (zombie.health + power - 1) / power;
        score += static_cast<double>(strikes) * zombie.danger;
      }
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
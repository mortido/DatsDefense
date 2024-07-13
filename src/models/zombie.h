#pragma once
#include <rapidjson/document.h>

#include <string>
#include <unordered_map>

#include "models/vec2i.h"

#ifdef DRAW
#include <rewind_viewer/RewindClient.h>
#include <rewind_viewer/colors.h>

#include "models/vec2d.h"
#endif

namespace mortido::models {
struct Zombie {
  enum class Type {
    normal,
    fast,
    bomber,
    liner,
    juggernaut,
    chaos_knight,
  };
  int attack;
  int health;
  int wait_turns;

  std::string id;
  Type type;
  vec2i velocity;
  vec2i position;

  void update_from_json(const rapidjson::Value& value) {
    static const std::unordered_map<std::string, Type> type_map = {
        {"normal", Type::normal},         {"fast", Type::fast},
        {"bomber", Type::bomber},         {"liner", Type::liner},
        {"juggernaut", Type::juggernaut}, {"chaos_knight", Type::chaos_knight},
    };

    attack = value["attack"].GetInt();
    health = value["health"].GetInt();
    wait_turns = value["waitTurns"].GetInt();
    id = value["id"].GetString();
    type = type_map.at(value["type"].GetString());
    int speed = value["speed"].GetInt();
    if (type == Type::chaos_knight) {
      speed = 1;  // todo: just in case...
    }
    std::string direction = value["direction"].GetString();
    if (direction == "up") {
      velocity.y = -speed;
    } else if (direction == "down") {
      velocity.y = speed;
    } else if (direction == "left") {
      velocity.x = -speed;
    } else if (direction == "right") {
      velocity.x = speed;
    }
    position = vec2i{value["x"].GetInt(), value["y"].GetInt()};
  }

  void move() {
    if (--wait_turns > 0) {
      return;
    }
    if (type == Type::chaos_knight) {
      position += velocity;
      position += velocity;
      velocity.rotate90ccw();
      position += velocity;
    } else {
      position += velocity;
    }
  }

  void move_2() {
    if (--wait_turns > 0) {
      return;
    }
    if (type == Type::chaos_knight) {
      position += velocity;
      position += velocity;
      velocity.rotate90cw();
      position += velocity;
    } else {
      position += velocity;
    }
  }

#ifdef DRAW
  vec2d calculate_offset() const {
    std::hash<std::string> hasher;
    size_t hash = hasher(id);
    double offset_x = 0.1 + 0.8 * (hash % 1000) / 999.0;  // Ensure offset_x is between 0.1 and 0.9
    double offset_y =
        0.1 + 0.8 * ((hash / 1000) % 1000) / 999.0;  // Ensure offset_y is between 0.1 and 0.9
    return vec2d{offset_x, offset_y};
  }

  void draw(rewind_viewer::RewindClient& rc) const {
    using namespace rewind_viewer::colors;

    // Define zombie colors
    constexpr uint32_t kNormalZombieColor = white::LavenderBlush;
    constexpr uint32_t kFastZombieColor = blue::DodgerBlue;
    constexpr uint32_t kBomberZombieColor = red::DarkRed;
    constexpr uint32_t kLinerZombieColor = purple::MediumPurple;
    constexpr uint32_t kJuggernautZombieColor = brown::SaddleBrown;
    constexpr uint32_t kChaosKnightZombieColor = orange::DarkOrange;

    uint32_t zombie_color;
    switch (type) {
      case Type::normal: zombie_color = kNormalZombieColor; break;
      case Type::fast: zombie_color = kFastZombieColor; break;
      case Type::bomber: zombie_color = kBomberZombieColor; break;
      case Type::liner: zombie_color = kLinerZombieColor; break;
      case Type::juggernaut: zombie_color = kJuggernautZombieColor; break;
      case Type::chaos_knight: zombie_color = kChaosKnightZombieColor; break;
    }

    vec2d pos = to_vec2d(position).add(calculate_offset());

    // Define the size of the triangle
    double size = 0.1;

    vec2d p1, p2, p3;
    double start_angle = 0;
    double end_angle = 0;
    if (velocity.x == 0 && velocity.y == 0) {
      // TODO: WTF?
      p1 = pos + vec2d{0, -size};
      p2 = pos + vec2d{size, size};
      p3 = pos + vec2d{-size, size};
    } else if (velocity.y < 0) {  // Moving up
      p1 = pos + vec2d{0, -size};
      p2 = pos + vec2d{size, size};
      p3 = pos + vec2d{-size, size};
      start_angle = 2 * M_PI;
      end_angle = M_PI;
    } else if (velocity.y > 0) {  // Moving down
      p1 = pos + vec2d{0, size};
      p2 = pos + vec2d{size, -size};
      p3 = pos + vec2d{-size, -size};
      start_angle = M_PI;
      end_angle = 0;
    } else if (velocity.x < 0) {  // Moving left
      p1 = pos + vec2d{-size, 0};
      p2 = pos + vec2d{size, size};
      p3 = pos + vec2d{size, -size};
      start_angle = 3 * M_PI / 2;
      end_angle = M_PI / 2;
    } else if (velocity.x > 0) {  // Moving right
      p1 = pos + vec2d{size, 0};
      p2 = pos + vec2d{-size, size};
      p3 = pos + vec2d{-size, -size};
      start_angle = M_PI / 2;
      end_angle = -M_PI / 2;
    }

    rc.switch_to_layer(5);
    rc.triangle(p1, p2, p3, zombie_color, true);
    vec2d popup_pos = p2.add(p3).mul(0.5);

    rc.arc(popup_pos, size, start_angle, end_angle, zombie_color, true);

    auto zombie_type_to_string = [](Type type) -> std::string {
      switch (type) {
        case Type::normal: return "Normal";
        case Type::fast: return "Fast";
        case Type::bomber: return "Bomber";
        case Type::liner: return "Liner";
        case Type::juggernaut: return "Juggernaut";
        case Type::chaos_knight: return "Chaos Knight";
        default: return "Unknown";
      }
    };

    // Draw popup with attack, health points, and zombie type
    std::string popup_text = zombie_type_to_string(type) + " Attack: " + std::to_string(attack) +
                             " Health: " + std::to_string(health) + " ID: " + id;

    rc.popup_round(popup_pos, size, popup_text.c_str());
  }
#endif
};
}  // namespace mortido::models
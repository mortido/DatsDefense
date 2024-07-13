#pragma once
#include <rapidjson/document.h>

#include <vector>

#include "logger.h"
#include "models/vec2i.h"
#include "models/zombie.h"

#ifdef DRAW
#include <rewind_viewer/RewindClient.h>
#include <rewind_viewer/colors.h>

#include "models/vec2d.h"
#endif

namespace mortido::models {

struct Building {
  int attack;
  int health;
  bool is_head;
  bool is_enemy;
  int range;

  std::string player_name;
  std::string id;
  std::optional<vec2i> last_attack;
  vec2i position;

  void update_from_json(const rapidjson::Value& value) {
    attack = value["attack"].GetInt();
    health = value["health"].GetInt();

    if (value.HasMember("isHead")) {
      is_head = value["isHead"].GetBool();
    } else {
      is_head = attack > 20;  // todo: wtf? =(
    }

    if (value.HasMember("range")) {
      range = value["range"].GetInt();
    } else {
      range = is_head ? 10 : 8;
    }

    if (value.HasMember("name")) {
      player_name = value["name"].GetString();
    }
    if (value.HasMember("id")) {
      id = value["id"].GetString();
    }

    if (value.HasMember("lastAttack") && value["lastAttack"].IsObject()) {
      vec2i last_attack_pos;
      last_attack_pos.x = value["lastAttack"]["x"].GetInt();
      last_attack_pos.y = value["lastAttack"]["y"].GetInt();
      last_attack = last_attack_pos;
    } else {
      last_attack.reset();
    }

    position.x = value["x"].GetInt();
    position.y = value["y"].GetInt();
  }

#ifdef DRAW
  void draw(rewind_viewer::RewindClient& rc, bool is_active) const {
    using namespace rewind_viewer::colors;

    // Define colors
    constexpr uint32_t kMyBuildingColor = green::SeaGreen;
    constexpr uint32_t kEnemyBuildingColor = orange::Tomato;
    constexpr uint32_t kMyAttackColor =  purple::Magenta;
    constexpr uint32_t kEnemyAttackColor = blue::Cyan;
    constexpr uint32_t kHeadBuildingCircleColor = yellow::Gold;
    constexpr uint32_t kHealthBarBackgroundColor = gray::DimGray;
    constexpr uint32_t kHealthBarForegroundColor = green::LimeGreen;

    rc.switch_to_layer(5);
    uint32_t color =        is_enemy ? kEnemyBuildingColor : kMyBuildingColor;
    if (!is_enemy && !is_active){
      rc.set_opacity(100);
    }
    rc.rectangle(to_vec2d(position), vec2d{1.0, 1.0}, color, true);
    rc.set_opacity(0xFF);

    if (is_head) {
      rc.circle(to_vec2d(position) + vec2d{0.5, 0.5}, 0.3, kHeadBuildingCircleColor, true);
      if (!is_enemy) {
        rc.camera_view("base", to_vec2d(position) + vec2d{0.5, 0.5}, 15.0);
      }
    }

    // Draw health bar
    int max_health = is_head ? 300 : 100;
    double health_ratio = static_cast<double>(health) / max_health;
    vec2d health_bar_pos = to_vec2d(position);
    vec2d health_bar_size = vec2d{1.0 * health_ratio, 0.1};
    rc.rectangle(health_bar_pos, vec2d{1.0, 0.1}, kHealthBarBackgroundColor, true);
    rc.rectangle(health_bar_pos, health_bar_size, kHealthBarForegroundColor, true);

    // Draw popup with attack and health points
    std::string popup_text =
        "Attack: " + std::to_string(attack) + " Health: " + std::to_string(health);
    rc.popup(to_vec2d(position), vec2d{1.0, 1.0}, popup_text.c_str());

    // Draw attack lines
    if (last_attack) {
      rc.switch_to_layer(6);
      vec2d start = to_vec2d(position) + vec2d{0.5, 0.5};
      vec2d aim = to_vec2d(*last_attack);
      vec2d end = aim + vec2d{0.5, 0.5};
      uint32_t attack_color = is_enemy ? kEnemyAttackColor : kMyAttackColor;
      rc.line(start, end, attack_color);
      rc.rectangle(aim, vec2d{1.0, 1.0}, attack_color, false);
    }
  }
#endif
};

}  // namespace mortido::models
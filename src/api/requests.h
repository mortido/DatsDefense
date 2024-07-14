#pragma once

#include <optional>
#include <vector>

#include "models/vec2i.h"

namespace mortido::api {
struct AttackCommand {
  std::string block_id;
  models::vec2i target;
  models::vec2i source;
};

struct Command {
  std::vector<AttackCommand> attack;
  std::vector<models::vec2i> build;
  std::optional<models::vec2i> move_base;
};

}  // namespace mortido::models
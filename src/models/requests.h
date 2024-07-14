#pragma once

#include <optional>
#include <vector>

#include "models/vec2i.h"

namespace mortido::models {
struct AttackCommand {
  std::string block_id;
  vec2i target;
  vec2i source;
};

struct Command {
  std::vector<AttackCommand> attack;
  std::vector<vec2i> build;
  std::optional<vec2i> move_base;
};

}  // namespace mortido::models
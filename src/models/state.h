#pragma once

#include <rapidjson/document.h>

#include "../logger.h"

namespace mortido::models {

struct State {
  static State from_json(const rapidjson::Document &json_state);
};

}

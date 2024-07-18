#pragma once
#include <rapidjson/document.h>

#include <exception>
#include <filesystem>

#include "requests.h"
#include "responses.h"

namespace mortido::api {

class ApiError : public std::runtime_error {
 public:
  explicit ApiError(const std::string& message) : std::runtime_error(message) {}
};

class Api {
 public:
  virtual Round get_current_round(const std::string& prev_round) = 0;
  virtual ParticipateResponse participate() = 0;
  virtual CommandResponse send_command(const Command& command) = 0;

  virtual rapidjson::Document get_world() = 0;
  virtual rapidjson::Document get_units() = 0;
  virtual bool active() = 0;
  virtual void set_dump_file(std::filesystem::path) {}
};

}  // namespace mortido::api

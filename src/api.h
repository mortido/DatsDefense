#pragma once
#include <rapidjson/document.h>

#include <chrono>
#include <list>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <exception>
#include <vector>

#include "logger.h"
#include "models/requests.h"
#include "models/responses.h"

namespace mortido::net {

class ApiError : public std::runtime_error {
 public:
  explicit ApiError(const std::string& message)
      : std::runtime_error(message) {}
};

class Api {
 public:
  explicit Api(std::string server_url, std::string token)
      : server_url_{std::move(server_url)}, token_{std::move(token)} {
    headers_.emplace_back("X-Auth-Token: " + token_);
    headers_.emplace_back("Accept: application/json");
  }

  rapidjson::Document get_rounds();
  rapidjson::Document participate();
  rapidjson::Document get_world();
  rapidjson::Document get_units();
  rapidjson::Document send_command(const models::Command &command);
  std::optional<models::Round> get_current_round(bool ignore_active);

 private:
  std::string server_url_;
  std::string token_;
  std::list<std::string> headers_;
  std::queue<std::chrono::steady_clock::time_point> request_times_;
  size_t max_retries_ = 30;
  size_t max_rps_ = 3;
  //  double max_rps_ = 4.0;
  //  std::chrono::duration<double> min_interval_ = std::chrono::duration<double>(1.0 / 4.0);

  rapidjson::Document get(const std::string &url) { return perform_request(url, "GET"); }
  rapidjson::Document post(const std::string &url, const std::string &body) {
    return perform_request(url, "POST", body);
  }
  rapidjson::Document put(const std::string &url, const std::string &body) {
    return perform_request(url, "PUT", body);
  }

  rapidjson::Document perform_request(const std::string &url, const std::string &method,
                                      const std::string &body = "");
  void ensure_rate_limit();
};

}  // namespace mortido::net

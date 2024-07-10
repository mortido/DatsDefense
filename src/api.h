#pragma once
#include <list>
#include <optional>
#include <string>
#include <utility>
#include <chrono>
#include <queue>
#include <vector>
#include <unordered_map>

#include <rapidjson/document.h>

#include "logger.h"

namespace mortido::net {

class Api {
 public:
  explicit Api(std::string server_url, std::string token) : server_url_{std::move(server_url)},
                                                             token_{std::move(token)} {
    headers_.emplace_back("X-Auth-Token: " + token_);
    headers_.emplace_back("Accept: application/json");
  }

//  std::optional<rapidjson::Document> universe();
//  std::optional<rapidjson::Document> rounds();
//  std::optional<rapidjson::Document> travel(const std::vector<std::string>& planets);
//  std::optional<rapidjson::Document> collect(const std::vector<models::Garbage>& garbage);
//  std::optional<rapidjson::Document> reset();
  std::optional<std::string> get_current_game_id();

 private:
  std::string server_url_;
  std::string token_;
  std::list<std::string> headers_;
  std::queue<std::chrono::steady_clock::time_point> request_times_;
  size_t max_retries_ = 10;
  size_t max_rpc_ = 4;

  std::optional<std::string> get(const std::string &url) {
    return perform_request(url, "GET");
  }
  std::optional<std::string> post(const std::string &url, const std::string &body) {
    return perform_request(url, "POST", body);
  }
  std::optional<std::string> delete_method(const std::string &url) {
    return perform_request(url, "DELETE");
  }
  std::optional<rapidjson::Document> parse_json(const std::optional<std::string>& json_string);
  std::optional<std::string> perform_request(const std::string &url,
                                             const std::string &method,
                                             const std::string &body = "");
  void ensure_rate_limit();
};

}

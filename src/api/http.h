#pragma once

#include <rapidjson/document.h>

#include <chrono>
#include <queue>
#include <string>
#include <thread>
#include <list>

#include "api/api.h"

namespace mortido::api {

class HttpApi : public Api {
 private:
  std::string server_url_;
  std::string token_;
  size_t max_rps_;
  size_t max_retries_;
  std::list<std::string> headers_;
  std::queue<std::chrono::steady_clock::time_point> request_times_;

 public:
  explicit HttpApi(std::string server_url, std::string token, size_t max_rps,
                   size_t max_retries = 50)
      : server_url_{std::move(server_url)}
      , token_{std::move(token)}
      , max_rps_{max_rps}
      , max_retries_{max_retries} {
    headers_.emplace_back("X-Auth-Token: " + token_);
    headers_.emplace_back("Accept: application/json");
  }

  ParticipateResponse participate() override;
  rapidjson::Document get_world() override;
  rapidjson::Document get_units() override;
  CommandResponse send_command(const Command &command) override;
  Round get_current_round(const std::string &prev_round) override;

 private:
  rapidjson::Document perform_request(const std::string &url, const std::string &method,
                                      const std::string &body = "");

  void ensure_rate_limit() {
    auto now = std::chrono::steady_clock::now();
    if (request_times_.size() < max_rps_) {
      request_times_.push(now);
      return;
    }

    auto oldest_request = request_times_.front();
    auto time_since_oldest_request = duration_cast<std::chrono::milliseconds>(now - oldest_request);
    if (time_since_oldest_request < std::chrono::seconds(1)) {
      auto delay_time =
          std::chrono::seconds(1) - time_since_oldest_request + std::chrono::milliseconds(10);
      std::this_thread::sleep_for(delay_time);
    }

    request_times_.push(now);
    request_times_.pop();
  }
};

}  // namespace mortido::api

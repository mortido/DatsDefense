#pragma once

#include <rapidjson/document.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <list>
#include <queue>
#include <string>
#include <thread>

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
  std::filesystem::path dump_file_name_;
  std::ofstream dump_file_;

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
  bool active() override { return true; }
  void set_dump_file(std::filesystem::path file_name) override{
    if (dump_file_.is_open()) {
      dump_file_.close();
    }
    dump_file_name_ = std::move(file_name);
  }

 private:
  rapidjson::Document perform_request(const std::string &url, const std::string &method,
                                      const std::string &body = "");
  void ensure_rate_limit();
  void dump_request(const std::string &handle, const std::string &method,
                    const std::string &request_data, long http_code,
                    const std::string &response_data);
};

}  // namespace mortido::api

#include "api/http.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <curlpp/Easy.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>

#include "logger.h"

using namespace std::chrono_literals;

namespace mortido::api {

ParticipateResponse HttpApi::participate() {
  auto json_response = perform_request("/play/zombidef/participate", "PUT");
  return ParticipateResponse::from_json(json_response);
}

rapidjson::Document HttpApi::get_world() {
  return perform_request("/play/zombidef/world", "GET");
}

rapidjson::Document HttpApi::get_units() {
  return perform_request("/play/zombidef/units", "GET");
}

CommandResponse HttpApi::send_command(const Command &command) {
  rapidjson::Document doc;
  doc.SetObject();
  auto &allocator = doc.GetAllocator();

  rapidjson::Value attack_array(rapidjson::kArrayType);
  for (const auto &attack_cmd : command.attack) {
    rapidjson::Value attack_obj(rapidjson::kObjectType);
    attack_obj.AddMember(
        "blockId", rapidjson::Value().SetString(attack_cmd.block_id.c_str(), allocator), allocator);

    rapidjson::Value target_obj(rapidjson::kObjectType);
    target_obj.AddMember("x", attack_cmd.target.x, allocator);
    target_obj.AddMember("y", attack_cmd.target.y, allocator);

    attack_obj.AddMember("target", target_obj, allocator);
    attack_array.PushBack(attack_obj, allocator);
  }
  doc.AddMember("attack", attack_array, allocator);

  rapidjson::Value build_array(rapidjson::kArrayType);
  for (const auto &build_pos : command.build) {
    rapidjson::Value build_obj(rapidjson::kObjectType);
    build_obj.AddMember("x", build_pos.x, allocator);
    build_obj.AddMember("y", build_pos.y, allocator);
    build_array.PushBack(build_obj, allocator);
  }
  doc.AddMember("build", build_array, allocator);

  if (command.move_base.has_value()) {
    rapidjson::Value move_base_obj(rapidjson::kObjectType);
    move_base_obj.AddMember("x", command.move_base->x, allocator);
    move_base_obj.AddMember("y", command.move_base->y, allocator);
    doc.AddMember("moveBase", move_base_obj, allocator);
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  auto json_response =
      perform_request("/play/zombidef/command", "POST", buffer.GetString());
  return CommandResponse::from_json(json_response);
}

Round HttpApi::get_current_round(const std::string &prev_round) {
  std::optional<Round> next_round;

  while (!next_round) {
    auto json_response = perform_request("/rounds/zombidef/", "GET");
    auto maybe_error = Error::from_json(json_response);
    if (maybe_error) {
      LOG_ERROR("Error [%d] during get_rounds: %s", maybe_error->err_code,
                maybe_error->message.c_str());
      continue;
    }

    auto rounds = RoundList::from_json(json_response);
    for (auto &round : rounds.rounds) {
      if (round.status == "active" && round.name != prev_round) {
        return round;
      }
      if (round.status == "not started" && round.start_at > rounds.now &&
          (!next_round || round.start_at < next_round->start_at)) {
        next_round = round;
      }
    }
    if (!next_round) {
      LOG_INFO("No next round, waiting 10 sec...");
      std::this_thread::sleep_for(10s);
    }
  }

  return *next_round;
}

rapidjson::Document HttpApi::perform_request(const std::string &handle, const std::string &method,
                                             const std::string &body) {
  std::string url = server_url_ + handle;

  for (size_t attempt = 0; attempt < max_retries_; ++attempt) {
    LOG_DEBUG("Request to %s attempt %zu", url.c_str(), attempt);
    ensure_rate_limit();

    try {
      std::ostringstream response_stream;
      curlpp::Easy request;
      request.setOpt<curlpp::options::Url>(url);
      request.setOpt<curlpp::options::HttpHeader>(headers_);
      request.setOpt<curlpp::options::WriteStream>(&response_stream);

      if (method == "POST") {
        request.setOpt<curlpp::options::PostFields>(body);
        request.setOpt<curlpp::options::PostFieldSize>(body.length());
      } else if (method == "PUT") {
        request.setOpt<curlpp::options::CustomRequest>("PUT");
        if (!body.empty()) {
          request.setOpt<curlpp::options::PostFields>(body);
          request.setOpt<curlpp::options::PostFieldSize>(body.length());
        }
      }
      request.perform();
      auto result = response_stream.str();
      long http_code = curlpp::infos::ResponseCode::get(request);
      if (http_code != 200) {
        LOG_WARN("%s HTTP response code: %ld result: %s", url.c_str(), http_code, result.c_str());
        if (http_code == 429) {
          // RPS limit
          continue;
        }
      }

      dump_request(handle,method,body,http_code,result);

      rapidjson::Document document;
      rapidjson::ParseResult parse_result = document.Parse(result.c_str());
      if (!parse_result) {
        LOG_ERROR("JSON parse error: %s, offset: %zu",
                  rapidjson::GetParseError_En(parse_result.Code()), parse_result.Offset());
        continue;
      }
      return document;
    } catch (curlpp::RuntimeError &e) {
      LOG_WARN("Runtime error on request to %s attempt %zu: %s", url.c_str(), attempt + 1,
               e.what());
    } catch (curlpp::LogicError &e) {
      LOG_WARN("Logic error on request to %s attempt %zu: %s", url.c_str(), attempt + 1, e.what());
    }
  }

  LOG_ERROR("Request was not executed %s", url.c_str());
  throw ApiError("Request was not executed");
}

void HttpApi::ensure_rate_limit() {
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

void HttpApi::dump_request(const std::string &handle, const std::string &method,
                           const std::string &request_data, long http_code,
                           const std::string &response_data) {
  if (dump_file_name_.empty()) {
    return;
  }
  if (!dump_file_.is_open()) {
    dump_file_.open(dump_file_name_, std::ios_base::app);
    if (!dump_file_.is_open()) {
      LOG_ERROR("Could not open error dump file for writing: %s", dump_file_name_.c_str());
      return;
    }
  }

  dump_file_ << "REQUEST " << method << " " << handle << std::endl;
  dump_file_ << request_data << std::endl;
  dump_file_ << "RESPONSE " << http_code << std::endl;
  dump_file_ << response_data << std::endl;
  dump_file_ << "~~~~~~~~~~~~~" << std::endl;
}

}  // namespace mortido::api

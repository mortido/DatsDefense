#include "api.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <chrono>
#include <curlpp/Easy.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iomanip>
#include <sstream>
#include <thread>

#include "models/json_utils.h"

namespace mortido::net {

std::optional<models::Round> Api::get_current_round(bool ignore_active) {
  auto doc = get_rounds();
  auto error = models::parse_error(doc);
  if (error) {
    LOG_ERROR("Error [%d] during get_rounds: %s", error->err_code, error->message.c_str());
    return std::nullopt;
  }

  auto rounds = models::parse_round_list(doc);
  auto now = rounds.now;

  std::optional<models::Round> next_round;
  for (auto &round : rounds.rounds) {
    round.now = now;
    if (round.status == "active" && !ignore_active) {
      return round;
    }

    if (round.status == "not started" && round.start_at > now &&
        (!next_round || round.start_at < next_round->start_at)) {
      next_round = round;
    }
  }

  return next_round;
}

rapidjson::Document Api::get_rounds() {
  return get(server_url_ + "/rounds/zombidef/");
}

rapidjson::Document Api::participate() {
  return put(server_url_ + "/play/zombidef/participate", "");
}

rapidjson::Document Api::get_world() {
//  rapidjson::Document document;
//  document.Parse("{\"realmName\":\"test-day1-9\",\"zpots\":[{\"x\":4,\"y\":495,\"type\":\"wall\"},{\"x\":0,\"y\":511,\"type\":\"wall\"},{\"x\":5,\"y\":495,\"type\":\"wall\"},{\"x\":0,\"y\":503,\"type\":\"default\"},{\"x\":0,\"y\":510,\"type\":\"default\"},{\"x\":15,\"y\":495,\"type\":\"default\"},{\"x\":8,\"y\":495,\"type\":\"default\"},{\"x\":15,\"y\":512,\"type\":\"default\"},{\"x\":0,\"y\":504,\"type\":\"default\"},{\"x\":14,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":513,\"type\":\"default\"},{\"x\":9,\"y\":510,\"type\":\"default\"},{\"x\":0,\"y\":502,\"type\":\"default\"},{\"x\":12,\"y\":510,\"type\":\"default\"},{\"x\":7,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":499,\"type\":\"default\"},{\"x\":11,\"y\":510,\"type\":\"default\"},{\"x\":13,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":508,\"type\":\"default\"},{\"x\":11,\"y\":495,\"type\":\"default\"},{\"x\":2,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":493,\"type\":\"default\"},{\"x\":7,\"y\":510,\"type\":\"default\"},{\"x\":8,\"y\":510,\"type\":\"default\"},{\"x\":0,\"y\":505,\"type\":\"default\"},{\"x\":0,\"y\":497,\"type\":\"default\"},{\"x\":14,\"y\":510,\"type\":\"default\"},{\"x\":0,\"y\":509,\"type\":\"default\"},{\"x\":0,\"y\":496,\"type\":\"default\"},{\"x\":10,\"y\":510,\"type\":\"default\"},{\"x\":3,\"y\":510,\"type\":\"default\"},{\"x\":1,\"y\":510,\"type\":\"default\"},{\"x\":5,\"y\":510,\"type\":\"default\"},{\"x\":3,\"y\":495,\"type\":\"default\"},{\"x\":15,\"y\":511,\"type\":\"default\"},{\"x\":13,\"y\":510,\"type\":\"default\"},{\"x\":2,\"y\":510,\"type\":\"default\"},{\"x\":0,\"y\":506,\"type\":\"default\"},{\"x\":0,\"y\":492,\"type\":\"default\"},{\"x\":10,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":501,\"type\":\"default\"},{\"x\":15,\"y\":513,\"type\":\"default\"},{\"x\":12,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":498,\"type\":\"default\"},{\"x\":9,\"y\":495,\"type\":\"default\"},{\"x\":6,\"y\":510,\"type\":\"default\"},{\"x\":1,\"y\":495,\"type\":\"default\"},{\"x\":0,\"y\":512,\"type\":\"default\"},{\"x\":6,\"y\":495,\"type\":\"default\"},{\"x\":4,\"y\":510,\"type\":\"default\"},{\"x\":0,\"y\":494,\"type\":\"default\"},{\"x\":0,\"y\":500,\"type\":\"default\"},{\"x\":15,\"y\":510,\"type\":\"default\"},{\"x\":0,\"y\":507,\"type\":\"default\"}]}");
//  return document;
  return get(server_url_ + "/play/zombidef/world");
}

rapidjson::Document Api::get_units() {
//  rapidjson::Document document;
//  document.Parse("{\n"
//      "  \"realmName\": \"test-day1-9\",\n"
//      "  \"player\": {\n"
//      "    \"gold\": 10,\n"
//      "    \"points\": 0,\n"
//      "    \"name\": \"Team Ö\",\n"
//      "    \"zombieKills\": 0,\n"
//      "    \"enemyBlockKills\": 0,\n"
//      "    \"gameEndedAt\": null\n"
//      "  },\n"
//      "  \"base\": [\n"
//      "    {\n"
//      "      \"id\": \"0190a843-4a6b-7946-b205-3adf3a20e993\",\n"
//      "      \"x\": 8,\n"
//      "      \"y\": 502,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 5,\n"
//      "      \"lastAttack\": {\"x\": 5, \"y\": 500}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"0190a843-4a6b-788d-b2e9-1171a434d158\",\n"
//      "      \"x\": 7,\n"
//      "      \"y\": 502,\n"
//      "      \"health\": 300,\n"
//      "      \"attack\": 40,\n"
//      "      \"range\": 8,\n"
//      "      \"isHead\": true,\n"
//      "      \"lastAttack\": {\"x\": 10, \"y\": 504}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"0190a843-5a6b-788d-b2e9-1171a434d158\",\n"
//      "      \"x\": 6,\n"
//      "      \"y\": 501,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 8,\n"
//      "      \"isHead\": false,\n"
//      "      \"lastAttack\": null\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"0190a843-4a6b-794d-816c-6d402b0ac2cd\",\n"
//      "      \"x\": 7,\n"
//      "      \"y\": 503,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 5,\n"
//      "      \"lastAttack\": {\"x\": 6, \"y\": 505}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"0190a843-4a6b-7956-8366-c07118e83587\",\n"
//      "      \"x\": 8,\n"
//      "      \"y\": 503,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 5,\n"
//      "      \"lastAttack\": {\"x\": 9, \"y\": 506}\n"
//      "    }\n"
//      "  ],\n"
//      "  \"zombies\": [\n"
//      "    {\n"
//      "      \"id\": \"zombie1\",\n"
//      "      \"x\": 5,\n"
//      "      \"y\": 500,\n"
//      "      \"health\": 5,\n"
//      "      \"attack\": 5,\n"
//      "      \"type\": \"normal\",\n"
//      "      \"direction\": \"down\",\n"
//      "      \"speed\": 1,\n"
//      "      \"waitTurns\": 0\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"zombie2\",\n"
//      "      \"x\": 10,\n"
//      "      \"y\": 504,\n"
//      "      \"health\": 5,\n"
//      "      \"attack\": 5,\n"
//      "      \"type\": \"fast\",\n"
//      "      \"direction\": \"left\",\n"
//      "      \"speed\": 2,\n"
//      "      \"waitTurns\": 0\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"zombie3\",\n"
//      "      \"x\": 6,\n"
//      "      \"y\": 505,\n"
//      "      \"health\": 5,\n"
//      "      \"attack\": 5,\n"
//      "      \"type\": \"bomber\",\n"
//      "      \"direction\": \"up\",\n"
//      "      \"speed\": 1,\n"
//      "      \"waitTurns\": 0\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"zombie4\",\n"
//      "      \"x\": 9,\n"
//      "      \"y\": 506,\n"
//      "      \"health\": 5,\n"
//      "      \"attack\": 5,\n"
//      "      \"type\": \"liner\",\n"
//      "      \"direction\": \"down\",\n"
//      "      \"speed\": 1,\n"
//      "      \"waitTurns\": 0\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"zombie5\",\n"
//      "      \"x\": 11,\n"
//      "      \"y\": 500,\n"
//      "      \"health\": 5,\n"
//      "      \"attack\": 5,\n"
//      "      \"type\": \"juggernaut\",\n"
//      "      \"direction\": \"left\",\n"
//      "      \"speed\": 1,\n"
//      "      \"waitTurns\": 0\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"zombie6\",\n"
//      "      \"x\": 10,\n"
//      "      \"y\": 501,\n"
//      "      \"health\": 5,\n"
//      "      \"attack\": 5,\n"
//      "      \"type\": \"chaos_knight\",\n"
//      "      \"direction\": \"up\",\n"
//      "      \"speed\": 1,\n"
//      "      \"waitTurns\": 0\n"
//      "    }\n"
//      "  ],\n"
//      "  \"enemyBlocks\": [\n"
//      "    {\n"
//      "      \"id\": \"enemy1\",\n"
//      "      \"x\": 4,\n"
//      "      \"y\": 500,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 5,\n"
//      "      \"isHead\": false,\n"
//      "      \"lastAttack\": {\"x\": 8, \"y\": 502}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"enemy2\",\n"
//      "      \"x\": 4,\n"
//      "      \"y\": 501,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 5,\n"
//      "      \"isHead\": false,\n"
//      "      \"lastAttack\": {\"x\": 7, \"y\": 502}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"enemy3\",\n"
//      "      \"x\": 4,\n"
//      "      \"y\": 502,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 5,\n"
//      "      \"isHead\": false,\n"
//      "      \"lastAttack\": {\"x\": 7, \"y\": 503}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"enemy4\",\n"
//      "      \"x\": 3,\n"
//      "      \"y\": 500,\n"
//      "      \"health\": 300,\n"
//      "      \"attack\": 40,\n"
//      "      \"range\": 8,\n"
//      "      \"isHead\": true,\n"
//      "      \"lastAttack\": {\"x\": 8, \"y\": 503}\n"
//      "    },\n"
//      "    {\n"
//      "      \"id\": \"enemy4\",\n"
//      "      \"x\": 2,\n"
//      "      \"y\": 499,\n"
//      "      \"health\": 100,\n"
//      "      \"attack\": 10,\n"
//      "      \"range\": 8,\n"
//      "      \"isHead\": false,\n"
//      "      \"lastAttack\": null\n"
//      "    }\n"
//      "  ],\n"
//      "  \"turnEndsInMs\": 0,\n"
//      "  \"turn\": 0\n"
//      "}");
//  return document;
  return get(server_url_ + "/play/zombidef/units");
}

rapidjson::Document Api::send_command(const models::Command &command) {
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
  return post(server_url_ + "/play/zombidef/command", buffer.GetString());
}

rapidjson::Document Api::perform_request(const std::string &url, const std::string &method,
                                         const std::string &body) {
  LOG_INFO("Request to %s", url.c_str());
  for (size_t attempt = 0; attempt < max_retries_; ++attempt) {
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
      LOG_INFO("Request to '%s' with data='%s' result='%s'", url.c_str(), body.c_str(),
               result.c_str());

      long http_code = curlpp::infos::ResponseCode::get(request);
      LOG_INFO("HTTP response code: %ld", http_code);

      if (http_code == 429) {
        LOG_WARN("Too many requests: %ld response code", http_code);
        continue;
      }

      rapidjson::Document document;
      rapidjson::ParseResult parse_result = document.Parse(result.c_str());
      if (!parse_result) {
        LOG_ERROR("JSON parse error: %s, offset: %zu",
                  rapidjson::GetParseError_En(parse_result.Code()), parse_result.Offset());
        throw ApiError("JSON parse error");
      }

      if (document.HasMember("error") && document["error"].IsString()) {
        LOG_ERROR("Request returned error: [%d] %s", document["errCode"].GetInt(),
                  document["error"].GetString());
        if (document["errCode"].GetInt() == 24) {
          continue;
        }
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

void Api::ensure_rate_limit() {
  auto now = std::chrono::steady_clock::now();
  if (request_times_.size() < max_rps_) {
    //  if (request_times_.size() < static_cast<size_t>(std::ceil(max_rps_))) {
    request_times_.push(now);
    return;
  }

  auto oldest_request = request_times_.front();
  auto time_since_oldest_request = duration_cast<std::chrono::milliseconds>(now - oldest_request);
  //  auto time_since_oldest_request = now - oldest_request;
  //  if (time_since_oldest_request < min_interval_) {
  //    auto delay_time = min_interval_ - time_since_oldest_request;
  //    std::this_thread::sleep_for(duration_cast<std::chrono::milliseconds>(delay_time));
  //  }
  if (time_since_oldest_request < std::chrono::seconds(1)) {
    auto delay_time = std::chrono::seconds(1) - time_since_oldest_request + std::chrono::milliseconds(10);
    std::this_thread::sleep_for(delay_time);
  }

  request_times_.push(now);
  request_times_.pop();
}

}  // namespace mortido::net

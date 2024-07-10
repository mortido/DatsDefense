#include "api.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <chrono>
#include <curlpp/Easy.hpp>
#include <curlpp/Exception.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iomanip>
#include <sstream>
#include <thread>

namespace mortido::net {

// std::optional<rapidjson::Document> Api::universe() {
//   return parse_json(get(server_url_ + "/player/universe"));
// }
//
// std::optional<rapidjson::Document> Api::rounds() {
//   return parse_json(get(server_url_ + "/player/rounds"));
// }

std::optional<std::string> Api::get_current_game_id() {
  //  auto doc = rounds();
  //
  //  if (!doc) {
  //    return std::nullopt;
  //  }
  //
  //  if (doc->HasMember("rounds") && (*doc)["rounds"].IsArray()) {
  //    for (const auto &round : (*doc)["rounds"].GetArray()) {
  //      if (round["isCurrent"].GetBool()) {
  //        return round["name"].GetString();
  //      }
  //    }
  //  }
  //  return std::nullopt;

  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);

  std::ostringstream oss;
  oss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d_%H-%M-%S");
  return oss.str();
}

// std::optional<rapidjson::Document> Api::travel(const std::vector<std::string> &planets) {
//   rapidjson::Document doc;
//   doc.SetObject();
//   rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
//   rapidjson::Value planets_array(rapidjson::kArrayType);
//   for (const auto &planet : planets) {
//     rapidjson::Value planet_value;
//     planet_value.SetString(planet.c_str(), static_cast<rapidjson::SizeType>(planet.length()),
//     allocator); planets_array.PushBack(planet_value, allocator);
//   }
//
//   doc.AddMember("planets", planets_array, allocator);
//   rapidjson::StringBuffer buffer;
//   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//   doc.Accept(writer);
//   return parse_json(post(server_url_ + "/player/travel", buffer.GetString()));
// }
//
// std::optional<rapidjson::Document> Api::collect(const std::vector<models::Garbage> &garbage) {
//   rapidjson::Document doc;
//   doc.SetObject();
//   rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
//
//   rapidjson::Value garbageObject(rapidjson::kObjectType);
//   for (const auto &g : garbage) {
//     rapidjson::Value key(g.name.c_str(), allocator);
//     rapidjson::Value points(rapidjson::kArrayType);
//     for (const auto &point : g.points) {
//       rapidjson::Value point_value(rapidjson::kArrayType);
//       point_value.PushBack(point.x, allocator);
//       point_value.PushBack(point.y, allocator);
//       points.PushBack(point_value, allocator);
//     }
//     garbageObject.AddMember(key, points, allocator);
//   }
//
//   doc.AddMember("garbage", garbageObject, allocator);
//   rapidjson::StringBuffer buffer;
//   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//   doc.Accept(writer);
//   return parse_json(post(server_url_ + "/player/collect", buffer.GetString()));
// }
//
// std::optional<rapidjson::Document> Api::reset() {
//   return parse_json(delete_method(server_url_ + "/player/reset"));
// }

std::optional<std::string> Api::perform_request(const std::string &url, const std::string &method,
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
      } else if (method == "DELETE" && !body.empty()) {
        request.setOpt<curlpp::options::CustomRequest>("DELETE");
        if (!body.empty()) {
          request.setOpt<curlpp::options::PostFields>(body);
          request.setOpt<curlpp::options::PostFieldSize>(body.length());
        }
      }
      request.perform();
      auto result = response_stream.str();
      LOG_INFO("Request to '%s' with data='%s' result='%s'", url.c_str(), body.c_str(),
               result.c_str());
      return result;
    } catch (curlpp::RuntimeError &e) {
      LOG_WARN("Runtime error on request to %s attempt %d: %s", url.c_str(), attempt + 1, e.what());
    } catch (curlpp::LogicError &e) {
      LOG_WARN("Logic error on request to %s attempt %d: %s", url.c_str(), attempt + 1, e.what());
    }
  }

  LOG_ERROR("Request was not executed %s", url.c_str());
  return std::nullopt;
}

std::optional<rapidjson::Document> Api::parse_json(const std::optional<std::string> &json_string) {
  if (!json_string.has_value()) {
    return std::nullopt;
  }

  rapidjson::Document document;
  rapidjson::ParseResult result = document.Parse(json_string.value().c_str());
  if (!result) {
    LOG_ERROR("JSON parse error: %s, offset: %zu", rapidjson::GetParseError_En(result.Code()),
              result.Offset());
    return std::nullopt;
  }

  if (document.HasMember("error") && document["error"].IsString()) {
    LOG_ERROR("Request returned error: %s", document["error"].GetString());
    //    return std::nullopt;
  }
  return document;
}

void Api::ensure_rate_limit() {
  if (request_times_.size() < max_rpc_) {
    request_times_.push(std::chrono::steady_clock::now());
    return;
  }

  auto now = std::chrono::steady_clock::now();
  auto oldest_request = request_times_.front();
  auto time_since_oldest_request = duration_cast<std::chrono::milliseconds>(now - oldest_request);

  if (time_since_oldest_request < std::chrono::seconds(1)) {
    auto delay_time = std::chrono::seconds(1) - time_since_oldest_request;
    std::this_thread::sleep_for(delay_time);
  }

  request_times_.push(std::chrono::steady_clock::now());
  request_times_.pop();
}

}  // namespace mortido::net

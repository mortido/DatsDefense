#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#include "api/dump_v2.h"
#include "api/http.h"
#include "game.h"
#include "logger.h"

using namespace std::chrono_literals;

constexpr const size_t kMaxRPS = 3;

constexpr const char *kServerURL = "https://games-test.datsteam.dev";
// constexpr const char *kServerURL = "https://games.datsteam.dev";

const std::filesystem::path kDataDir = "data";
constexpr const char *kTokenFile = "token.txt";
constexpr const char *kReplayFile = "test-map2-45.dump";
constexpr const char *kMainDumpFile = "main.dump";
constexpr const char *kMainLogFile = "main.log";

std::string read_token() {
  std::string token;
  std::ifstream file(kDataDir / kTokenFile);

  if (!file.is_open()) {
    LOG_FATAL("Unable to open file: %s", (kDataDir / kTokenFile).c_str());
    std::exit(EXIT_FAILURE);
  }

  if (!(file >> token)) {
    LOG_FATAL("Failed to read token from file: %s", (kDataDir / kTokenFile).c_str());
    std::exit(EXIT_FAILURE);
  }

  file.close();
  return token;
}

std::string format_time_point_as_local_time(const std::chrono::system_clock::time_point &tp) {
  std::time_t time = std::chrono::system_clock::to_time_t(tp);
  std::tm local_tm = *std::localtime(&time);
  std::ostringstream oss;
  oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

int main(int argc, char *argv[]) {
  loguru::init(argc, argv);
  loguru::g_flush_interval_ms = 0;  // unbuffered
  loguru::add_file((kDataDir / kMainLogFile).c_str(), loguru::Append, loguru::Verbosity_WARNING);

  //  mortido::api::HttpApi api(kServerURL, read_token(), kMaxRPS, 30);
  mortido::api::DumpApi api(kDataDir / kReplayFile);
  std::string prev_round_name;

  while (api.active()) {
    api.set_dump_file(kDataDir / kMainDumpFile);
    auto round = api.get_current_round(prev_round_name);
    LOG_INFO("ROUND %s DURATION: %.1f min", round.name.c_str(),
             static_cast<double>(round.duration) / 60.0);

    while (round.status != "active") {
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(round.start_at - round.now) + 2s;
      auto local_start_time = std::chrono::system_clock::now() + duration;
      std::string start_time_str = format_time_point_as_local_time(local_start_time);

      LOG_INFO("Waiting %lld milliseconds until the start of the next game: %s at %s",
               duration.count(), round.name.c_str(), start_time_str.c_str());
      std::this_thread::sleep_for(duration);
      round = api.get_current_round(prev_round_name);
    }

    LOG_INFO("Game %s has started.", round.name.c_str());
    api.set_dump_file((kDataDir / round.name).replace_extension(".dump"));
    const auto game_log_file = (kDataDir / round.name).replace_extension(".log");
    loguru::add_file(game_log_file.c_str(), loguru::Append, loguru::Verbosity_MAX);
    mortido::game::Game game(round.name, api);
    while (!game.run()) {
      LOG_ERROR("GAME %s FINISHED WITH NEGATIVE RESULT, RELOADING...", round.name.c_str());
    }
    LOG_INFO("GAME %s FINISHED", round.name.c_str());
    loguru::remove_callback(game_log_file.c_str());
    loguru::flush();
    prev_round_name = round.name;
  }

  loguru::remove_all_callbacks();
  loguru::flush();

  return EXIT_SUCCESS;
}

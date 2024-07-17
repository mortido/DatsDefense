#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>


#include "api/http.h"
#include "api/dump.h"
#include "game.h"
#include "logger.h"


using namespace std::chrono_literals;

constexpr const size_t kMaxRPS = 3;
constexpr const char *kTokenFile = "data/token.txt";
constexpr const char *kServerURL = "https://games-test.datsteam.dev";
//constexpr const char *kServerURL = "https://games.datsteam.dev";
constexpr const char *kReplayFile = "data/game_test-map1-30.dump";
constexpr const bool kWriteDump = true;
//constexpr const bool kWriteDump = false;

std::string read_token() {
  std::string token;
  std::ifstream file(kTokenFile);

  if (!file.is_open()) {
    LOG_FATAL("Unable to open file: %s", kTokenFile);
    std::exit(EXIT_FAILURE);
  }

  if (!(file >> token)) {
    LOG_FATAL("Failed to read token from file: %s", kTokenFile);
    std::exit(EXIT_FAILURE);
  }

  file.close();
  return token;
}

std::string format_time_point_as_local_time(const std::chrono::system_clock::time_point& tp) {
  std::time_t time = std::chrono::system_clock::to_time_t(tp);
  std::tm local_tm = *std::localtime(&time);
  std::ostringstream oss;
  oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

int main(int argc, char *argv[]) {
  loguru::init(argc, argv);
  loguru::g_flush_interval_ms = 0;  // unbuffered
  mortido::api::HttpApi api(kServerURL, read_token(), kMaxRPS, 30);
//  mortido::api::DumpApi api(kReplayFile);
  std::string prev_round_name;

  while (true) {
    auto round = api.get_current_round(prev_round_name);
//    auto round = api.get_current_round(false);
//    if (!round || round->name == prev_round_name){
//      LOG_INFO("ACTIVE ROUND IS THE SAME, GETTING NEXT ONE...");
//      round = api.get_current_round(true);
//    }
//
//    while (!round) {
//      LOG_INFO("NO NEXT ROUND, WAITING 10 sec...");
//      std::this_thread::sleep_for(10s);
//      round = api.get_current_round(true);
//    }

//    LOG_INFO("ROUND %s DURATION: %.1f min", round->name.c_str(), static_cast<double>(round->duration)/ 60.0);
    LOG_INFO("ROUND %s DURATION: %.1f min", round.name.c_str(), static_cast<double>(round.duration)/ 60.0);

    if (round.start_at > round.now) {
//    if (round->start_at > round->now) {
//      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(round->start_at - round->now);
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(round.start_at - round.now) + 2s;
      auto local_start_time = std::chrono::system_clock::now() + duration;
      std::string start_time_str = format_time_point_as_local_time(local_start_time);

      LOG_INFO("Waiting %lld milliseconds until the start of the next game: %s at %s",
               duration.count(),
               round.name.c_str(),
               start_time_str.c_str());
      std::this_thread::sleep_for(duration);
//      auto r = api.get_current_round(false);
//      while(!r && r->name!=round->name && r->status != "active") {
//        LOG_INFO("Wait a little more");
//        std::this_thread::sleep_for(100ms);
//        r = api.get_current_round(false);
//      }
      LOG_INFO("Game %s has started.", round.name.c_str());
    }
    mortido::game::Game game(round.name, api, kWriteDump);
    while (!game.run()) {
      LOG_ERROR("GAME %s FINISHED WITH NEGATIVE RESULT, RELOADING...", round.name.c_str());
    }

    LOG_INFO("GAME %s FINISHED", round.name.c_str());
    prev_round_name = round.name;
  }

  loguru::remove_all_callbacks();
  loguru::flush();

  return EXIT_SUCCESS;
}

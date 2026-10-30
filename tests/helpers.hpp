// Shared test helpers: load fixtures, a mock host and input scripts.
#pragma once

#include <chili_beat/engine.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "check.hpp"

namespace helpers {

inline std::string read(const std::string& name) {
  std::ifstream in(std::string(CHILI_BEAT_DATA) + "/" + name, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}

inline chili_beat::Game fixture(const std::string& name) {
  chili_beat::Game game;
  std::string error;
  bool ok = chili_beat::load(read(name), game, error);
  if (!ok) std::cerr << name << ": " << error << "\n";
  CHECK(ok);
  return game;
}

// Records frames; buttons and time come from the test.
class MockHost : public chili_beat::Host {
 public:
  uint32_t delta = 1000 / 60;
  uint8_t held = 0;
  std::vector<chili_beat::Frame> frames;

  uint32_t delta_time_ms() override { return delta; }
  uint8_t buttons() override { return held; }
  void present(const chili_beat::Frame& frame) override { frames.push_back(frame); }
};

// Runs an input script. Each word is a button press held for one step, then
// released for one step: "up", "down", "left", "right", "a", "b". "wait" waits
// one step, "beat" waits until the next world beat, "N" repeats the next word N times.
inline void play(chili_beat::Engine& engine, const std::string& script) {
  std::istringstream words(script);
  std::string word;
  int repeat = 1;
  while (words >> word) {
    if (word[0] >= '0' && word[0] <= '9') {
      repeat = std::stoi(word);
      continue;
    }
    for (int i = 0; i < repeat; ++i) {
      using namespace chili_beat;
      if (word == "wait") {
        engine.step(0);
      } else if (word == "beat") {
        uint32_t start = engine.beat();
        while (engine.beat() == start) engine.step(0);
      } else {
        uint8_t b = word == "up" ? kUp : word == "down" ? kDown : word == "left" ? kLeft
                  : word == "right" ? kRight : word == "a" ? kA : word == "b" ? kB : 0;
        CHECK(b != 0);
        engine.step(b);
        engine.step(0);
      }
    }
    repeat = 1;
  }
}

// Cell of the first thing with this key, or -1.
inline int find(const chili_beat::Engine& engine, char key) {
  for (int i = 0; i < chili_beat::kCells; ++i) {
    const auto& slot = engine.board()[i];
    if (slot && engine.game().things[slot->kind].key == key) return i;
  }
  return -1;
}

inline int cell(int x, int y) { return y * chili_beat::kBoardSize + x; }

}  // namespace helpers

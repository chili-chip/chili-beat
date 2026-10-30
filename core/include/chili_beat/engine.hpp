// Runs one game. See docs/beat.md for the exact behaviour.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "game.hpp"
#include "host.hpp"

namespace chili_beat {

constexpr int kStepsPerSecond = 60;

// A thing on the board.
struct Thing {
  int8_t kind = kNone;
  Dir dir = Dir::Down;
  uint16_t id = 0;        // unique while the game runs
  uint8_t from = 0;       // cell it slides from
  uint32_t moved_at = 0;  // step of the last move, for the slide
};

using Board = std::array<std::optional<Thing>, kCells>;

class Engine {
 public:
  explicit Engine(Game game);

  // Advances by the host's time in fixed steps, then presents a frame.
  void update(Host& host);
  // One fixed step (1/60 s) with the buttons held now.
  void step(uint8_t buttons);
  void draw(Frame& frame) const;
  // Starts the game again from the beginning.
  void restart();

  const Game& game() const { return game_; }
  const Board& board() const { return screens_[screen_].board; }
  int screen() const { return screen_; }
  uint32_t beat() const { return beat_; }
  uint32_t steps() const { return step_; }

 private:
  struct ScreenState {
    Board board;
    bool visited = false;
  };

  bool is_you(const Thing& t) const { return game_.things[t.kind].you; }
  void enter(int screen, std::optional<Thing> walker, int cell);
  void press(uint8_t button);
  void world_beat();
  void clock_beat();
  bool move(int cell, Dir dir);
  void run_actions(const Rule& rule, uint8_t button);

  Game game_;
  std::vector<ScreenState> screens_;
  int screen_ = 0;
  uint16_t next_id_ = 1;
  uint32_t step_ = 0;
  uint32_t beat_ = 0;
  uint32_t clock_beats_ = 0;
  int clock_ = 0;
  uint32_t time_ = 0;  // ms * 60, for update()
  uint8_t held_ = 0;
  // A walk to another screen, done after the current rule.
  struct Walk {
    int screen;
    int cell;
    Thing thing;
  };
  std::optional<Walk> walk_;
};

}  // namespace chili_beat

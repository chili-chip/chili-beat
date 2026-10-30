// Runs one game. See docs/beat.md for the exact behaviour.
#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "game.hpp"
#include "host.hpp"

namespace chili_beat {

constexpr int kStepsPerSecond = 60;
constexpr int kMaxBumpChain = 16;
constexpr int kTextColumns = 20;
constexpr int kTextLines = 3;

// A thing on the board.
struct Thing {
  int8_t kind = kNone;
  Dir dir = Dir::Down;
  uint32_t id = 0;          // unique while the game runs
  uint8_t from = 0;         // cell it slides from
  uint32_t moved_at = 0;    // step of the last move, for the slide
  uint32_t moved_beat = 0;  // world beat of the last move during a world beat
};

using Board = std::array<std::optional<Thing>, kCells>;

class Engine {
 public:
  enum class State : uint8_t { Playing, Won, Lost };

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
  State state() const { return state_; }
  int counter(int index) const { return counters_[index]; }
  // The open text box page, or empty. Lines are split by '\n'.
  std::string text() const { return pages_.empty() ? std::string() : pages_.front(); }
  // Rules that ran during the last step, by index, in order.
  const std::vector<uint8_t>& fired() const { return fired_; }
  // Blips started during the last step, by index.
  const std::vector<uint8_t>& sounds() const { return sounds_; }

 private:
  struct ScreenState {
    Board board;
    bool visited = false;
  };

  // What a running rule refers to.
  struct Context {
    Trigger::Type trigger = Trigger::Type::Start;
    uint32_t it = 0;
    uint32_t bumper = 0;
    uint32_t bumped = 0;
    std::optional<Dir> way;  // arrow pressed or bump direction
    int depth = 0;           // bumps so far in this chain
  };

  Board& live() { return screens_[screen_].board; }
  bool is_you(const Thing& t) const { return game_.things[t.kind].you; }
  bool matches(const Match& m, int8_t kind) const;
  int cell_of(uint32_t id) const;
  bool playing() const { return state_ == State::Playing && pages_.empty(); }

  void enter(int screen, std::optional<Thing> walker, int cell);
  void settle();
  void run_trigger(Trigger::Type type);
  void counter_rules();
  void press(uint8_t button);
  void world_beat();
  void fire(int rule, const Context& ctx);
  bool conditions_hold(const Rule& rule);
  void act(const Action& action, const Context& ctx);
  std::optional<Dir> resolve(Way way, int cell, const Context& ctx);
  bool move(int cell, Dir dir, int depth);
  void say(const std::string& text);
  uint32_t random();

  Game game_;
  std::vector<ScreenState> screens_;
  int screen_ = 0;
  uint32_t next_id_ = 1;
  uint32_t step_ = 0;
  uint32_t beat_ = 0;
  uint32_t clock_beats_ = 0;
  int clock_ = 0;
  uint32_t time_ = 0;  // ms * 60, for update()
  uint8_t held_ = 0;
  uint32_t rng_ = 1;
  bool in_beat_ = false;
  uint32_t shake_until_ = 0;
  State state_ = State::Playing;
  std::array<int, kMaxCounters> counters_{};
  std::array<int, kMaxCounters> seen_{};
  std::deque<std::string> pages_;
  std::vector<uint8_t> fired_;
  std::vector<uint8_t> sounds_;
  // A screen change, done after the current rule.
  struct Change {
    int screen;
    int cell;                    // walk: entry cell
    std::optional<Thing> thing;  // walk: the thing that walked
  };
  std::optional<Change> change_;
};

}  // namespace chili_beat

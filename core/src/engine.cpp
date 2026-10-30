#include <chili_beat/engine.hpp>

#include <string>
#include <utility>

#include "font.hpp"

namespace chili_beat {
namespace {

constexpr int kDx[] = {0, 1, 0, -1};
constexpr int kDy[] = {-1, 0, 1, 0};

// Cell next to cell in dir, or -1 off the board.
int neighbour(int cell, Dir dir) {
  int x = cell % kBoardSize + kDx[static_cast<int>(dir)];
  int y = cell / kBoardSize + kDy[static_cast<int>(dir)];
  if (x < 0 || y < 0 || x >= kBoardSize || y >= kBoardSize) return -1;
  return y * kBoardSize + x;
}

// The cell where a thing walking off cell in dir enters the next screen.
int entry_cell(int cell, Dir dir) {
  int x = cell % kBoardSize;
  int y = cell / kBoardSize;
  switch (dir) {
    case Dir::Up: y = kBoardSize - 1; break;
    case Dir::Down: y = 0; break;
    case Dir::Left: x = kBoardSize - 1; break;
    case Dir::Right: x = 0; break;
  }
  return y * kBoardSize + x;
}

void draw_picture(Frame& frame, const Picture& pic, int px, int py, bool see_through) {
  for (int y = 0; y < kCellPixels; ++y) {
    int sy = py + y;
    if (sy < 0 || sy >= kScreenPixels) continue;
    for (int x = 0; x < kCellPixels; ++x) {
      int sx = px + x;
      uint8_t c = pic[y * kCellPixels + x];
      if (sx < 0 || sx >= kScreenPixels || (see_through && c == 0)) continue;
      frame.pixels[sy * kScreenPixels + sx] = c;
    }
  }
}

void fill(Frame& frame, int x0, int y0, int w, int h, uint8_t colour) {
  for (int y = y0; y < y0 + h; ++y) {
    for (int x = x0; x < x0 + w; ++x) {
      if (x >= 0 && y >= 0 && x < kScreenPixels && y < kScreenPixels) {
        frame.pixels[y * kScreenPixels + x] = colour;
      }
    }
  }
}

void draw_text(Frame& frame, std::string_view text, int px, int py, uint8_t colour) {
  for (char c : text) {
    const uint8_t* rows = font::glyph(c);
    for (int y = 0; y < font::kHeight; ++y) {
      for (int x = 0; x < 5; ++x) {
        if (rows[y] & (0x10 >> x)) fill(frame, px + x, py + y, 1, 1, colour);
      }
    }
    px += font::kWidth;
  }
}

}  // namespace

Engine::Engine(Game game) : game_(std::move(game)) { restart(); }

void Engine::restart() {
  screens_.assign(game_.screens.size(), ScreenState{});
  next_id_ = 1;
  beat_ = 0;
  clock_ = 0;
  walk_.reset();
  screen_ = game_.start;
  enter(game_.start, std::nullopt, 0);
}

void Engine::enter(int index, std::optional<Thing> walker, int cell) {
  // You things belong to the screen you are on, so they leave with you.
  for (auto& slot : screens_[screen_].board) {
    if (slot && is_you(*slot)) slot.reset();
  }
  screen_ = index;
  ScreenState& state = screens_[index];
  const Screen& screen = game_.screens[index];
  if (!state.visited || screen.reset) {
    state.board = {};
    for (int i = 0; i < kCells; ++i) {
      int8_t kind = screen.things[i];
      if (kind != kNone && !game_.things[kind].you) {
        state.board[i] = Thing{kind, Dir::Down, next_id_++, static_cast<uint8_t>(i), 0};
      }
    }
  }
  state.visited = true;
  if (walker) {
    walker->from = static_cast<uint8_t>(cell);
    state.board[cell] = *walker;
    return;
  }
  for (int i = 0; i < kCells; ++i) {
    int8_t kind = screen.things[i];
    if (kind != kNone && game_.things[kind].you && !state.board[i]) {
      state.board[i] = Thing{kind, Dir::Down, next_id_++, static_cast<uint8_t>(i), 0};
    }
  }
}

void Engine::update(Host& host) {
  time_ += host.delta_time_ms() * kStepsPerSecond;
  uint8_t buttons = host.buttons();
  // Catch up at most a quarter second, so a long pause doesn't freeze the host.
  for (int i = 0; time_ >= 1000 && i < kStepsPerSecond / 4; ++i) {
    time_ -= 1000;
    step(buttons);
  }
  if (time_ >= 1000) time_ = 0;
  Frame frame;
  draw(frame);
  host.present(frame);
}

void Engine::step(uint8_t buttons) {
  uint8_t pressed = buttons & ~held_;
  held_ = buttons;
  ++step_;

  bool any = false;
  for (uint8_t button : {kA, kB, kUp, kDown, kLeft, kRight}) {
    if (pressed & button) {
      any = true;
      press(button);
    }
  }
  if (game_.turns && any) world_beat();

  clock_ += game_.tempo;
  if (clock_ >= kStepsPerSecond) {
    clock_ -= kStepsPerSecond;
    clock_beat();
    if (!game_.turns) world_beat();
  }
}

void Engine::press(uint8_t button) {
  for (const Rule& rule : game_.rules) {
    if (rule.when.type == Trigger::Type::Press && (rule.when.buttons & button)) run_actions(rule, button);
  }
  if (walk_) {
    Walk walk = *walk_;
    walk_.reset();
    enter(walk.screen, walk.thing, walk.cell);
  }
}

void Engine::world_beat() { ++beat_; }

void Engine::clock_beat() { ++clock_beats_; }

void Engine::run_actions(const Rule& rule, uint8_t button) {
  for (const Action& action : rule.actions) {
    if (action.type != Action::Type::Move) continue;
    Dir dir = Dir::Down;
    switch (button) {
      case kUp: dir = Dir::Up; break;
      case kDown: dir = Dir::Down; break;
      case kLeft: dir = Dir::Left; break;
      case kRight: dir = Dir::Right; break;
    }
    if (action.way <= Way::Left) dir = static_cast<Dir>(action.way);
    // Move each you thing once, from a list taken before any of them move.
    std::vector<uint16_t> ids;
    for (const auto& slot : board()) {
      if (slot && is_you(*slot)) ids.push_back(slot->id);
    }
    for (uint16_t id : ids) {
      for (int cell = 0; cell < kCells; ++cell) {
        const auto& slot = board()[cell];
        if (slot && slot->id == id) {
          move(cell, dir);
          break;
        }
      }
    }
  }
}

bool Engine::move(int cell, Dir dir) {
  Board& board = screens_[screen_].board;
  Thing& thing = *board[cell];
  thing.dir = dir;
  int target = neighbour(cell, dir);
  if (target < 0) {
    if (!is_you(thing) || walk_) return false;
    const Screen& here = game_.screens[screen_];
    int mx = here.map_x + kDx[static_cast<int>(dir)];
    int my = here.map_y + kDy[static_cast<int>(dir)];
    for (size_t i = 0; i < game_.screens.size(); ++i) {
      const Screen& next = game_.screens[i];
      if (next.map_x != mx || next.map_y != my) continue;
      int entry = entry_cell(cell, dir);
      const ScreenState& state = screens_[i];
      bool fresh = !state.visited || next.reset;
      bool full = fresh ? next.things[entry] != kNone && !game_.things[next.things[entry]].you
                        : state.board[entry].has_value();
      if (full) return false;
      walk_ = Walk{static_cast<int>(i), entry, thing};
      board[cell].reset();
      return true;
    }
    return false;
  }
  if (board[target]) return false;  // a bump; rules come in M2
  thing.from = static_cast<uint8_t>(cell);
  thing.moved_at = step_;
  board[target] = thing;
  board[cell].reset();
  return true;
}

void Engine::draw(Frame& frame) const {
  frame.pixels.fill(0);
  const Screen& screen = game_.screens[screen_];
  auto picture = [&](int8_t kind) -> const Picture& {
    const auto& frames = game_.things[kind].frames;
    return frames[clock_beats_ % frames.size()];
  };
  for (int i = 0; i < kCells; ++i) {
    if (screen.ground[i] != kNone) {
      draw_picture(frame, picture(screen.ground[i]), i % kBoardSize * kCellPixels, i / kBoardSize * kCellPixels,
                   false);
    }
  }
  // A move slides over a quarter beat: 60 / (tempo * 4) steps.
  for (int i = 0; i < kCells; ++i) {
    const auto& slot = board()[i];
    if (!slot) continue;
    int x = i % kBoardSize * kCellPixels;
    int y = i / kBoardSize * kCellPixels;
    int left = kStepsPerSecond - static_cast<int>(step_ - slot->moved_at) * game_.tempo * 4;
    if (slot->moved_at && left > 0) {
      x += (slot->from % kBoardSize * kCellPixels - x) * left / kStepsPerSecond;
      y += (slot->from / kBoardSize * kCellPixels - y) * left / kStepsPerSecond;
    }
    draw_picture(frame, picture(slot->kind), x, y, true);
  }
  // Counters: icon and number on a colour 0 strip, top left.
  int x = 0;
  for (const Counter& counter : game_.counters) {
    std::string number = std::to_string(counter.start);
    int width = kCellPixels + 1 + static_cast<int>(number.size()) * font::kWidth;
    fill(frame, x, 0, width + 1, kCellPixels + 1, 0);
    draw_picture(frame, game_.things[counter.icon].frames[0], x, 0, true);
    draw_text(frame, number, x + kCellPixels + 1, 0, 3);
    x += width + 2;
  }
}

}  // namespace chili_beat

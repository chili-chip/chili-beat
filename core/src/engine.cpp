#include <chili_beat/engine.hpp>

#include <algorithm>
#include <cstdlib>
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

uint32_t Engine::random() {
  // mulberry32: small, fast and the same on every platform.
  uint32_t z = (rng_ += 0x6D2B79F5u);
  z = (z ^ (z >> 15)) * (z | 1u);
  z ^= z + (z ^ (z >> 7)) * (z | 61u);
  return z ^ (z >> 14);
}

void Engine::restart() {
  screens_.assign(game_.screens.size(), ScreenState{});
  next_id_ = 1;
  beat_ = 0;
  clock_ = 0;
  rng_ = game_.seed;
  in_beat_ = false;
  shake_until_ = 0;
  state_ = State::Playing;
  pages_.clear();
  change_.reset();
  for (size_t i = 0; i < game_.counters.size(); ++i) counters_[i] = seen_[i] = game_.counters[i].start;
  screen_ = game_.start;
  enter(game_.start, std::nullopt, 0);
  run_trigger(Trigger::Type::Start);
  settle();
}

void Engine::enter(int index, std::optional<Thing> walker, int cell) {
  // You things belong to the screen you are on, so they leave with you.
  for (auto& slot : live()) {
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
        state.board[i] = Thing{kind, Dir::Down, next_id_++, static_cast<uint8_t>(i), 0, 0};
      }
    }
  }
  state.visited = true;
  if (walker) {
    walker->from = static_cast<uint8_t>(cell);
    walker->moved_at = 0;
    state.board[cell] = *walker;
    return;
  }
  for (int i = 0; i < kCells; ++i) {
    int8_t kind = screen.things[i];
    if (kind != kNone && game_.things[kind].you && !state.board[i]) {
      state.board[i] = Thing{kind, Dir::Down, next_id_++, static_cast<uint8_t>(i), 0, 0};
    }
  }
}

// Runs counter rules, then any screen change and the new screen's start rules.
void Engine::settle() {
  counter_rules();
  if (!change_ || state_ != State::Playing) {
    change_.reset();
    return;
  }
  Change change = std::move(*change_);
  change_.reset();
  enter(change.screen, change.thing, change.cell);
  run_trigger(Trigger::Type::Start);
  counter_rules();
}

void Engine::run_trigger(Trigger::Type type) {
  Context ctx;
  ctx.trigger = type;
  for (size_t i = 0; i < game_.rules.size() && state_ == State::Playing; ++i) {
    if (game_.rules[i].when.type == type) fire(static_cast<int>(i), ctx);
  }
}

void Engine::counter_rules() {
  for (int pass = 0; pass < 16 && state_ == State::Playing; ++pass) {
    bool changed = false;
    for (size_t c = 0; c < game_.counters.size(); ++c) {
      if (counters_[c] == seen_[c]) continue;
      seen_[c] = counters_[c];
      changed = true;
      Context ctx;
      ctx.trigger = Trigger::Type::Counter;
      for (size_t i = 0; i < game_.rules.size() && state_ == State::Playing; ++i) {
        const Trigger& when = game_.rules[i].when;
        if (when.type == Trigger::Type::Counter && when.counter == static_cast<int8_t>(c) &&
            when.value == counters_[c]) {
          fire(static_cast<int>(i), ctx);
        }
      }
    }
    if (!changed) return;
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
  fired_.clear();
  sounds_.clear();

  if (!pages_.empty()) {
    if (pressed & kA) {
      pages_.pop_front();
      if (pages_.empty() && state_ != State::Playing) restart();
    }
  } else if (state_ == State::Playing) {
    bool any = false;
    for (uint8_t button : {kA, kB, kUp, kDown, kLeft, kRight}) {
      if (!(pressed & button) || !playing()) continue;
      any = true;
      press(button);
      settle();
    }
    if (game_.turns && any && playing()) world_beat();
  }

  clock_ += game_.tempo;
  if (clock_ >= kStepsPerSecond) {
    clock_ -= kStepsPerSecond;
    ++clock_beats_;
    if (!game_.turns && playing()) world_beat();
  }
}

void Engine::press(uint8_t button) {
  Context ctx;
  ctx.trigger = Trigger::Type::Press;
  if (button == kUp) ctx.way = Dir::Up;
  if (button == kRight) ctx.way = Dir::Right;
  if (button == kDown) ctx.way = Dir::Down;
  if (button == kLeft) ctx.way = Dir::Left;
  for (size_t i = 0; i < game_.rules.size() && state_ == State::Playing; ++i) {
    const Trigger& when = game_.rules[i].when;
    if (when.type == Trigger::Type::Press && (when.buttons & button)) fire(static_cast<int>(i), ctx);
  }
}

void Engine::world_beat() {
  ++beat_;
  in_beat_ = true;
  std::vector<uint32_t> ids;
  for (const auto& slot : board()) {
    if (slot) ids.push_back(slot->id);
  }
  for (uint32_t id : ids) {
    for (size_t i = 0; i < game_.rules.size() && state_ == State::Playing; ++i) {
      const Trigger& when = game_.rules[i].when;
      if (when.type != Trigger::Type::Every || beat_ % when.every != 0) continue;
      int cell = cell_of(id);
      if (cell < 0) break;
      if (!matches(when.thing, board()[cell]->kind)) continue;
      Context ctx;
      ctx.trigger = Trigger::Type::Every;
      ctx.it = id;
      fire(static_cast<int>(i), ctx);
    }
  }
  in_beat_ = false;
  settle();
}

bool Engine::matches(const Match& m, int8_t kind) const {
  switch (m.type) {
    case Match::Type::Any: return true;
    case Match::Type::You: return game_.things[kind].you;
    case Match::Type::Kind: return m.kind == kind;
  }
  return false;
}

int Engine::cell_of(uint32_t id) const {
  for (int i = 0; i < kCells; ++i) {
    if (board()[i] && board()[i]->id == id) return i;
  }
  return -1;
}

void Engine::fire(int index, const Context& ctx) {
  const Rule& rule = game_.rules[index];
  if (!conditions_hold(rule)) return;
  fired_.push_back(static_cast<uint8_t>(index));
  for (const Action& action : rule.actions) act(action, ctx);
}

bool Engine::conditions_hold(const Rule& rule) {
  for (const Condition& c : rule.conditions) {
    switch (c.type) {
      case Condition::Type::Chance:
        if (random() % static_cast<uint32_t>(c.value) != 0) return false;
        break;
      case Condition::Type::AtLeast:
        if (counters_[c.counter] < c.value) return false;
        break;
      case Condition::Type::AtMost:
        if (counters_[c.counter] > c.value) return false;
        break;
      case Condition::Type::Exactly:
        if (counters_[c.counter] != c.value) return false;
        break;
    }
  }
  return true;
}

void Engine::act(const Action& action, const Context& ctx) {
  using T = Action::Type;
  switch (action.type) {
    case T::Count: {
      int& value = counters_[action.target];
      value = action.set ? action.value : value + action.value;
      value = std::clamp(value, 0, kMaxCount);
      return;
    }
    case T::Say: say(action.text); return;
    case T::Sound: sounds_.push_back(static_cast<uint8_t>(action.target)); return;
    case T::Go:
      if (!change_) change_ = Change{action.target, 0, std::nullopt};
      return;
    case T::Shake: shake_until_ = step_ + kStepsPerSecond / game_.tempo; return;
    case T::Win:
    case T::Lose:
      if (state_ != State::Playing) return;
      state_ = action.type == T::Win ? State::Won : State::Lost;
      say(!action.text.empty() ? action.text : action.type == T::Win ? "You win!" : "You lose.");
      return;
    default: break;
  }

  // The rest change things. Pick them before any of them change.
  Who who = action.who;
  if (who == Who::Default) {
    who = ctx.trigger == Trigger::Type::Every  ? Who::It
          : ctx.trigger == Trigger::Type::Bump ? Who::Bumper
                                               : Who::You;
  }
  std::vector<uint32_t> ids;
  if (who == Who::You) {
    for (const auto& slot : board()) {
      if (slot && is_you(*slot)) ids.push_back(slot->id);
    }
  } else {
    ids.push_back(who == Who::It ? ctx.it : who == Who::Bumper ? ctx.bumper : ctx.bumped);
  }

  for (uint32_t id : ids) {
    int cell = cell_of(id);
    if (cell < 0) continue;
    Thing& thing = *live()[cell];
    switch (action.type) {
      case T::Move: {
        if (in_beat_ && thing.moved_beat == beat_) break;
        if (auto dir = resolve(action.way, cell, ctx)) move(cell, *dir, ctx.depth);
        break;
      }
      case T::Turn:
        if (auto dir = resolve(action.way, cell, ctx)) thing.dir = *dir;
        break;
      case T::Become: thing.kind = action.target; break;
      case T::Vanish: live()[cell].reset(); break;
      case T::Make: {
        auto dir = resolve(action.way, cell, ctx);
        if (!dir) break;
        int target = neighbour(cell, *dir);
        if (target >= 0 && !live()[target]) {
          live()[target] = Thing{action.target, *dir, next_id_++, static_cast<uint8_t>(target), 0, 0};
        }
        break;
      }
      default: break;
    }
  }
}

std::optional<Dir> Engine::resolve(Way way, int cell, const Context& ctx) {
  const Thing& thing = *board()[cell];
  auto opposite = [](Dir d) { return static_cast<Dir>((static_cast<int>(d) + 2) % 4); };
  switch (way) {
    case Way::Up: return Dir::Up;
    case Way::Right: return Dir::Right;
    case Way::Down: return Dir::Down;
    case Way::Left: return Dir::Left;
    case Way::Forward: return thing.dir;
    case Way::Back: return opposite(thing.dir);
    case Way::Same: return ctx.way ? *ctx.way : thing.dir;
    case Way::Random: return static_cast<Dir>(random() % 4);
    case Way::Toward:
    case Way::Away: {
      int x = cell % kBoardSize, y = cell / kBoardSize;
      int best = -1, best_distance = 1 << 30;
      for (int i = 0; i < kCells; ++i) {
        const auto& slot = board()[i];
        if (!slot || i == cell || !is_you(*slot)) continue;
        int distance = std::abs(i % kBoardSize - x) + std::abs(i / kBoardSize - y);
        if (distance < best_distance) {
          best = i;
          best_distance = distance;
        }
      }
      if (best < 0) return std::nullopt;
      int dx = best % kBoardSize - x, dy = best / kBoardSize - y;
      Dir dir = std::abs(dx) >= std::abs(dy) ? (dx > 0 ? Dir::Right : Dir::Left) : (dy > 0 ? Dir::Down : Dir::Up);
      return way == Way::Toward ? dir : opposite(dir);
    }
  }
  return std::nullopt;
}

bool Engine::move(int cell, Dir dir, int depth) {
  Board& board = live();
  Thing& thing = *board[cell];
  thing.dir = dir;
  int target = neighbour(cell, dir);
  if (target < 0) {
    if (!is_you(thing) || change_) return false;
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
      change_ = Change{static_cast<int>(i), entry, thing};
      board[cell].reset();
      return true;
    }
    return false;
  }

  if (board[target]) {
    if (depth >= kMaxBumpChain) return false;
    Context ctx;
    ctx.trigger = Trigger::Type::Bump;
    ctx.bumper = thing.id;
    ctx.bumped = board[target]->id;
    ctx.way = dir;
    ctx.depth = depth + 1;
    for (size_t i = 0; i < game_.rules.size() && state_ == State::Playing; ++i) {
      const Trigger& when = game_.rules[i].when;
      if (when.type != Trigger::Type::Bump) continue;
      int bumper = cell_of(ctx.bumper);
      int bumped = cell_of(ctx.bumped);
      if (bumper < 0 || bumped < 0) break;
      if (matches(when.bumper, board[bumper]->kind) && matches(when.bumped, board[bumped]->kind)) {
        fire(static_cast<int>(i), ctx);
      }
    }
    // The bumper moves in if it is still in place and the cell is free now.
    if (!board[cell] || board[cell]->id != ctx.bumper || board[target]) return false;
  }

  Thing moving = *board[cell];
  moving.from = static_cast<uint8_t>(cell);
  moving.moved_at = step_;
  if (in_beat_) moving.moved_beat = beat_;
  board[target] = moving;
  board[cell].reset();
  return true;
}

// Splits text into lines of up to 20 characters at spaces, and lines into pages of 3.
void Engine::say(const std::string& text) {
  std::vector<std::string> lines;
  std::string line;
  size_t i = 0;
  while (i < text.size()) {
    size_t end = text.find(' ', i);
    if (end == std::string::npos) end = text.size();
    std::string word = text.substr(i, end - i);
    i = end + 1;
    while (word.size() > static_cast<size_t>(kTextColumns)) {
      if (!line.empty()) lines.push_back(std::move(line));
      lines.push_back(word.substr(0, kTextColumns));
      line.clear();
      word = word.substr(kTextColumns);
    }
    if (word.empty()) continue;
    if (!line.empty() && line.size() + 1 + word.size() > static_cast<size_t>(kTextColumns)) {
      lines.push_back(std::move(line));
      line.clear();
    }
    line += (line.empty() ? "" : " ") + word;
  }
  if (!line.empty() || lines.empty()) lines.push_back(line);
  for (size_t l = 0; l < lines.size(); l += kTextLines) {
    std::string page;
    for (size_t k = l; k < l + kTextLines && k < lines.size(); ++k) page += (k > l ? "\n" : "") + lines[k];
    pages_.push_back(std::move(page));
  }
}

void Engine::draw(Frame& frame) const {
  frame.pixels.fill(0);
  const Screen& screen = game_.screens[screen_];
  auto picture = [&](int8_t kind) -> const Picture& {
    const auto& frames = game_.things[kind].frames;
    return frames[clock_beats_ % frames.size()];
  };
  // A shake moves the board one pixel left and right every 2 steps.
  int shake = step_ < shake_until_ ? (step_ / 2 % 2 ? 1 : -1) : 0;
  for (int i = 0; i < kCells; ++i) {
    if (screen.ground[i] != kNone) {
      draw_picture(frame, picture(screen.ground[i]), i % kBoardSize * kCellPixels + shake,
                   i / kBoardSize * kCellPixels, false);
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
    draw_picture(frame, picture(slot->kind), x + shake, y, true);
  }
  // Counters: icon and number on a colour 0 strip, top left.
  int x = 0;
  for (size_t c = 0; c < game_.counters.size(); ++c) {
    std::string number = std::to_string(counters_[c]);
    int width = kCellPixels + 1 + static_cast<int>(number.size()) * font::kWidth;
    fill(frame, x, 0, width + 1, kCellPixels + 1, 0);
    draw_picture(frame, game_.things[game_.counters[c].icon].frames[0], x, 0, true);
    draw_text(frame, number, x + kCellPixels + 1, 0, 3);
    x += width + 2;
  }
  // Text box: 3 lines of 20 characters near the bottom.
  if (!pages_.empty()) {
    constexpr int kBoxX = 2, kBoxY = 92, kBoxW = 124, kBoxH = 34;
    fill(frame, kBoxX, kBoxY, kBoxW, kBoxH, 3);
    fill(frame, kBoxX + 1, kBoxY + 1, kBoxW - 2, kBoxH - 2, 0);
    const std::string& page = pages_.front();
    int line = 0;
    size_t start = 0;
    while (start <= page.size()) {
      size_t end = page.find('\n', start);
      if (end == std::string::npos) end = page.size();
      draw_text(frame, std::string_view(page).substr(start, end - start), kBoxX + 4,
                kBoxY + 4 + line * (font::kHeight + 1), 3);
      ++line;
      start = end + 1;
    }
    // A blinking arrow when A shows another page.
    if (pages_.size() > 1 && clock_beats_ % 2 == 0) draw_text(frame, ">", kBoxX + kBoxW - 8, kBoxY + kBoxH - 10, 3);
  }
}

}  // namespace chili_beat

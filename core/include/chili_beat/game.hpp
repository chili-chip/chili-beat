// The game data, as loaded from a .beat file. See docs/format.md.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace chili_beat {

constexpr int kBoardSize = 16;
constexpr int kCells = kBoardSize * kBoardSize;
constexpr int kCellPixels = 8;
constexpr int kScreenPixels = kBoardSize * kCellPixels;  // 128
constexpr int kMapSize = 8;
constexpr int kMaxThings = 64;
constexpr int kMaxScreens = 64;
constexpr int kMaxCounters = 4;
constexpr int kMaxRules = 128;
constexpr int kMaxConditions = 2;
constexpr int kMaxActions = 6;
constexpr int kMaxBlips = 16;
constexpr int kMaxTunes = 8;
constexpr int kTuneSteps = 16;
constexpr int kMaxCount = 999;
constexpr int kMaxText = 64;
constexpr int8_t kNone = -1;

// Sweetie 16 by GrafxKid, as 0xRRGGBB.
constexpr std::array<uint32_t, 16> kSweetie16 = {
    0x1a1c2c, 0x5d275d, 0xb13e53, 0xef7d57, 0xffcd75, 0xa7f070, 0x38b764, 0x257179,
    0x29366f, 0x3b5dc9, 0x41a6f6, 0x73eff7, 0xf4f4f4, 0x94b0c2, 0x566c86, 0x333c57,
};

enum Button : uint8_t {
  kUp = 1 << 0,
  kDown = 1 << 1,
  kLeft = 1 << 2,
  kRight = 1 << 3,
  kA = 1 << 4,
  kB = 1 << 5,
  kArrows = kUp | kDown | kLeft | kRight,
};

enum class Dir : uint8_t { Up, Right, Down, Left };

enum class Way : uint8_t { Up, Right, Down, Left, Forward, Back, Same, Toward, Away, Random };

enum class Who : uint8_t { Default, You, It, Bumper, Bumped };

using Picture = std::array<uint8_t, kCellPixels * kCellPixels>;  // colours 0-3, row by row

struct ThingKind {
  char key = '?';
  std::string name;
  bool you = false;
  std::vector<Picture> frames;  // 1 or 2
};

struct Screen {
  std::string id;
  int map_x = 0;
  int map_y = 0;
  int8_t tune = kNone;
  bool reset = false;
  std::array<int8_t, kCells> things{};  // thing kind index or kNone
  std::array<int8_t, kCells> ground{};
};

struct Counter {
  std::string id;
  std::string name;
  int8_t icon = 0;  // thing kind index
  int start = 0;
};

// A thing pattern in a bump or beat trigger.
struct Match {
  enum class Type : uint8_t { Kind, You, Any };
  Type type = Type::Any;
  int8_t kind = kNone;
};

struct Trigger {
  enum class Type : uint8_t { Start, Press, Bump, Every, Counter };
  Type type = Type::Start;
  uint8_t buttons = 0;  // Press: Button mask
  Match bumper;         // Bump
  Match bumped;         // Bump
  Match thing;          // Every
  int every = 1;        // Every
  int8_t counter = kNone;
  int value = 0;  // Counter: reaches
};

struct Condition {
  enum class Type : uint8_t { Chance, AtLeast, AtMost, Exactly };
  Type type = Type::Chance;
  int8_t counter = kNone;
  int value = 0;
};

struct Action {
  enum class Type : uint8_t { Move, Turn, Become, Vanish, Make, Count, Say, Sound, Go, Shake, Win, Lose };
  Type type = Type::Move;
  Who who = Who::Default;
  Way way = Way::Same;
  int8_t target = kNone;  // kind (become, make), counter, blip or screen
  int value = 0;          // Count
  bool set = false;       // Count: set instead of add
  std::string text;       // Say, Win, Lose
};

struct Rule {
  Trigger when;
  std::vector<Condition> conditions;
  std::vector<Action> actions;
};

struct Blip {
  enum class Preset : uint8_t { Coin, Jump, Hit, Boom, Step, Power, Blip };
  std::string id;
  Preset preset = Preset::Blip;
  int pitch = 4;
  int length = 3;
  int slide = 0;
};

struct Tune {
  enum class Scale : uint8_t { Major, Minor, PentatonicMajor, PentatonicMinor };
  std::string id;
  Scale scale = Scale::Major;
  std::array<uint8_t, kTuneSteps> lead{};
  std::array<uint8_t, kTuneSteps> beat{};
};

struct Game {
  std::string title;
  std::string author;
  std::array<uint8_t, 4> palette{};
  int tempo = 4;
  bool turns = false;
  uint32_t seed = 1;
  int8_t start = 0;  // screen index
  std::vector<Counter> counters;
  std::vector<ThingKind> things;
  std::vector<Screen> screens;
  std::vector<Rule> rules;
  std::vector<Blip> blips;
  std::vector<Tune> tunes;
};

// Loads a .beat file. On failure, returns false and sets error to one clear message.
bool load(std::string_view text, Game& game, std::string& error);

}  // namespace chili_beat

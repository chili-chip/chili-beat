// Turns a .beat file into a Game, checking everything format/schema.json checks
// plus the references between parts. See docs/format.md.
#include <chili_beat/game.hpp>

#include <initializer_list>
#include <string>

#include "json.hpp"

namespace chili_beat {
namespace {

using json::Value;
using Type = Value::Type;

struct Loader {
  Game& game;
  std::string error;

  bool fail(const std::string& where, const std::string& message) {
    if (error.empty()) error = where + ": " + message;
    return false;
  }

  // Checks that obj is an object with only the given fields.
  bool object(const Value& v, const std::string& where, std::initializer_list<std::string_view> fields) {
    if (v.type != Type::Object) return fail(where, "must be an object");
    for (const auto& [key, _] : v.members) {
      bool known = false;
      for (auto f : fields) known |= key == f;
      if (!known) return fail(where, "unknown field \"" + key + "\"");
    }
    return true;
  }

  const Value* field(const Value& obj, std::string_view key, const std::string& where, bool required) {
    const Value* v = obj.get(key);
    if (!v && required) fail(where, "missing \"" + std::string(key) + "\"");
    return v;
  }

  bool integer(const Value* v, const std::string& where, int64_t min, int64_t max, int64_t& out) {
    if (v->type != Type::Int) return fail(where, "must be a whole number");
    if (v->integer < min || v->integer > max) {
      return fail(where, "must be from " + std::to_string(min) + " to " + std::to_string(max));
    }
    out = v->integer;
    return true;
  }

  bool integer(const Value& obj, std::string_view key, const std::string& where, int min, int max,
               int& out, bool required = true) {
    const Value* v = field(obj, key, where, required);
    if (!v) return required ? false : true;
    int64_t n;
    if (!integer(v, where + "." + std::string(key), min, max, n)) return false;
    out = static_cast<int>(n);
    return true;
  }

  static size_t text_length(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) n += (c & 0xC0) != 0x80;
    return n;
  }

  bool text(const Value& obj, std::string_view key, const std::string& where, size_t min, size_t max,
            std::string& out, bool required = true) {
    const Value* v = field(obj, key, where, required);
    if (!v) return !required;
    std::string at = where + "." + std::string(key);
    if (v->type != Type::String) return fail(at, "must be text");
    size_t n = text_length(v->string);
    if (n < min || n > max) {
      return fail(at, "must be " + std::to_string(min) + " to " + std::to_string(max) + " characters");
    }
    out = v->string;
    return true;
  }

  bool boolean(const Value& obj, std::string_view key, const std::string& where, bool& out) {
    const Value* v = obj.get(key);
    if (!v) return true;
    if (v->type != Type::Bool) return fail(where + "." + std::string(key), "must be true or false");
    out = v->boolean;
    return true;
  }

  bool must_be_true(const Value& v, const std::string& where) {
    if (v.type != Type::Bool || !v.boolean) return fail(where, "must be true");
    return true;
  }

  bool id(const Value& obj, std::string_view key, const std::string& where, std::string& out,
          bool required = true) {
    if (!text(obj, key, where, 1, 16, out, required)) return false;
    for (char c : out) {
      if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) {
        return fail(where + "." + std::string(key), "ids use only a-z, 0-9 and -");
      }
    }
    return true;
  }

  bool array(const Value* v, const std::string& where, size_t min, size_t max) {
    if (v->type != Type::Array) return fail(where, "must be a list");
    if (v->items.size() < min || v->items.size() > max) {
      return fail(where, "must have " + std::to_string(min) + " to " + std::to_string(max) + " items");
    }
    return true;
  }

  static bool is_key(char c) { return c > ' ' && c <= '~' && c != '.'; }

  int8_t kind_of(char key) const {
    for (size_t i = 0; i < game.things.size(); ++i) {
      if (game.things[i].key == key) return static_cast<int8_t>(i);
    }
    return kNone;
  }

  bool kind(const Value* v, const std::string& where, int8_t& out) {
    if (v->type != Type::String || v->string.size() != 1 || !is_key(v->string[0])) {
      return fail(where, "must be a thing key");
    }
    out = kind_of(v->string[0]);
    if (out == kNone) return fail(where, "no thing has key \"" + v->string + "\"");
    return true;
  }

  template <typename T>
  bool find(const Value* v, const std::string& where, const std::vector<T>& list, const char* what,
            int8_t& out) {
    if (v->type != Type::String) return fail(where, std::string("must be a ") + what + " id");
    for (size_t i = 0; i < list.size(); ++i) {
      if (list[i].id == v->string) {
        out = static_cast<int8_t>(i);
        return true;
      }
    }
    return fail(where, std::string("no ") + what + " has id \"" + v->string + "\"");
  }

  template <typename T>
  bool unique(const std::vector<T>& list, const std::string& where) {
    for (size_t i = 0; i < list.size(); ++i) {
      for (size_t j = 0; j < i; ++j) {
        if (list[i].id == list[j].id) return fail(where, "id \"" + list[i].id + "\" is used twice");
      }
    }
    return true;
  }

  template <typename E>
  bool word(const Value* v, const std::string& where, std::initializer_list<std::string_view> words,
            E& out) {
    if (v->type == Type::String) {
      int i = 0;
      for (auto w : words) {
        if (v->string == w) {
          out = static_cast<E>(i);
          return true;
        }
        ++i;
      }
    }
    std::string list;
    for (auto w : words) list += (list.empty() ? "" : ", ") + std::string(w);
    return fail(where, "must be one of: " + list);
  }

  bool picture(const Value& v, const std::string& where, Picture& out) {
    if (!array(&v, where, 8, 8)) return false;
    for (int y = 0; y < 8; ++y) {
      const Value& row = v.items[y];
      if (row.type != Type::String || row.string.size() != 8) {
        return fail(where + "[" + std::to_string(y) + "]", "must be 8 digits");
      }
      for (int x = 0; x < 8; ++x) {
        char c = row.string[x];
        if (c < '0' || c > '3') return fail(where + "[" + std::to_string(y) + "]", "digits must be 0-3");
        out[y * 8 + x] = static_cast<uint8_t>(c - '0');
      }
    }
    return true;
  }

  bool layer(const Value* v, const std::string& where, std::array<int8_t, kCells>& out) {
    if (!array(v, where, kBoardSize, kBoardSize)) return false;
    for (int y = 0; y < kBoardSize; ++y) {
      const Value& row = v->items[y];
      std::string at = where + "[" + std::to_string(y) + "]";
      if (row.type != Type::String || row.string.size() != kBoardSize) {
        return fail(at, "must be 16 characters");
      }
      for (int x = 0; x < kBoardSize; ++x) {
        char c = row.string[x];
        if (c == '.') {
          out[y * kBoardSize + x] = kNone;
          continue;
        }
        out[y * kBoardSize + x] = kind_of(c);
        if (out[y * kBoardSize + x] == kNone) {
          return fail(at, std::string("no thing has key \"") + c + "\"");
        }
      }
    }
    return true;
  }

  bool match(const Value* v, const std::string& where, Match& out) {
    if (v->type == Type::String && v->string == "you") {
      out.type = Match::Type::You;
      return true;
    }
    if (v->type == Type::String && v->string == "any") {
      out.type = Match::Type::Any;
      return true;
    }
    out.type = Match::Type::Kind;
    return kind(v, where, out.kind);
  }

  bool thing(const Value& v, const std::string& where) {
    if (!object(v, where, {"key", "name", "you", "frames"})) return false;
    ThingKind t;
    const Value* key = field(v, "key", where, true);
    if (!key) return false;
    if (key->type != Type::String || key->string.size() != 1 || !is_key(key->string[0])) {
      return fail(where + ".key", "must be one character, not . or a space");
    }
    t.key = key->string[0];
    if (kind_of(t.key) != kNone) return fail(where + ".key", "key \"" + key->string + "\" is used twice");
    if (!text(v, "name", where, 1, 16, t.name) || !boolean(v, "you", where, t.you)) return false;
    const Value* frames = field(v, "frames", where, true);
    if (!frames || !array(frames, where + ".frames", 1, 2)) return false;
    t.frames.resize(frames->items.size());
    for (size_t i = 0; i < t.frames.size(); ++i) {
      if (!picture(frames->items[i], where + ".frames[" + std::to_string(i) + "]", t.frames[i])) return false;
    }
    game.things.push_back(std::move(t));
    return true;
  }

  bool screen(const Value& v, const std::string& where) {
    if (!object(v, where, {"id", "map", "tune", "reset", "things", "ground"})) return false;
    Screen s;
    s.ground.fill(kNone);
    if (!id(v, "id", where, s.id) || !boolean(v, "reset", where, s.reset)) return false;
    const Value* map = field(v, "map", where, true);
    if (!map || !array(map, where + ".map", 2, 2)) return false;
    int64_t x, y;
    if (!integer(&map->items[0], where + ".map[0]", 0, kMapSize - 1, x) ||
        !integer(&map->items[1], where + ".map[1]", 0, kMapSize - 1, y)) {
      return false;
    }
    s.map_x = static_cast<int>(x);
    s.map_y = static_cast<int>(y);
    for (const auto& other : game.screens) {
      if (other.map_x == s.map_x && other.map_y == s.map_y) {
        return fail(where + ".map", "screen \"" + other.id + "\" is already in this place");
      }
    }
    const Value* things = field(v, "things", where, true);
    if (!things || !layer(things, where + ".things", s.things)) return false;
    if (const Value* ground = v.get("ground")) {
      if (!layer(ground, where + ".ground", s.ground)) return false;
    }
    if (const Value* tune = v.get("tune")) {
      if (!find(tune, where + ".tune", game.tunes, "tune", s.tune)) return false;
    }
    game.screens.push_back(std::move(s));
    return true;
  }

  bool counter(const Value& v, const std::string& where) {
    if (!object(v, where, {"id", "name", "icon", "start"})) return false;
    Counter c;
    if (!id(v, "id", where, c.id) || !text(v, "name", where, 1, 16, c.name) ||
        !integer(v, "start", where, 0, kMaxCount, c.start, false)) {
      return false;
    }
    const Value* icon = field(v, "icon", where, true);
    if (!icon || !kind(icon, where + ".icon", c.icon)) return false;
    game.counters.push_back(std::move(c));
    return true;
  }

  bool blip(const Value& v, const std::string& where) {
    if (!object(v, where, {"id", "preset", "pitch", "length", "slide"})) return false;
    Blip b;
    const Value* preset = field(v, "preset", where, true);
    if (!id(v, "id", where, b.id) || !preset ||
        !word(preset, where + ".preset", {"coin", "jump", "hit", "boom", "step", "power", "blip"}, b.preset) ||
        !integer(v, "pitch", where, 0, 7, b.pitch, false) ||
        !integer(v, "length", where, 0, 7, b.length, false) ||
        !integer(v, "slide", where, 0, 7, b.slide, false)) {
      return false;
    }
    game.blips.push_back(std::move(b));
    return true;
  }

  bool steps(const Value& v, std::string_view key, const std::string& where, int max,
             std::array<uint8_t, kTuneSteps>& out) {
    const Value* list = field(v, key, where, true);
    std::string at = where + "." + std::string(key);
    if (!list || !array(list, at, kTuneSteps, kTuneSteps)) return false;
    for (int i = 0; i < kTuneSteps; ++i) {
      int64_t n;
      if (!integer(&list->items[i], at + "[" + std::to_string(i) + "]", 0, max, n)) return false;
      out[i] = static_cast<uint8_t>(n);
    }
    return true;
  }

  bool tune(const Value& v, const std::string& where) {
    if (!object(v, where, {"id", "scale", "lead", "beat"})) return false;
    Tune t;
    const Value* scale = field(v, "scale", where, true);
    if (!id(v, "id", where, t.id) || !scale ||
        !word(scale, where + ".scale", {"major", "minor", "pentatonic-major", "pentatonic-minor"}, t.scale) ||
        !steps(v, "lead", where, 15, t.lead) || !steps(v, "beat", where, 10, t.beat)) {
      return false;
    }
    game.tunes.push_back(std::move(t));
    return true;
  }

  bool trigger(const Value& v, const std::string& where, Trigger& t) {
    if (v.type != Type::Object) return fail(where, "must be an object");
    if (const Value* start = v.get("start")) {
      t.type = Trigger::Type::Start;
      return object(v, where, {"start"}) && must_be_true(*start, where + ".start");
    }
    if (const Value* press = v.get("press")) {
      t.type = Trigger::Type::Press;
      constexpr uint8_t masks[] = {kA, kB, kArrows, kUp, kDown, kLeft, kRight};
      int i = 0;
      if (!object(v, where, {"press"}) ||
          !word(press, where + ".press", {"a", "b", "arrow", "up", "down", "left", "right"}, i)) {
        return false;
      }
      t.buttons = masks[i];
      return true;
    }
    if (const Value* bump = v.get("bump")) {
      t.type = Trigger::Type::Bump;
      return object(v, where, {"bump"}) && array(bump, where + ".bump", 2, 2) &&
             match(&bump->items[0], where + ".bump[0]", t.bumper) &&
             match(&bump->items[1], where + ".bump[1]", t.bumped);
    }
    if (v.get("every")) {
      t.type = Trigger::Type::Every;
      const Value* thing = field(v, "thing", where, true);
      return object(v, where, {"every", "thing"}) && integer(v, "every", where, 1, 64, t.every) && thing &&
             match(thing, where + ".thing", t.thing);
    }
    if (const Value* counter = v.get("counter")) {
      t.type = Trigger::Type::Counter;
      return object(v, where, {"counter", "reaches"}) &&
             find(counter, where + ".counter", game.counters, "counter", t.counter) &&
             integer(v, "reaches", where, 0, kMaxCount, t.value);
    }
    return fail(where, "needs one of start, press, bump, every, counter");
  }

  bool condition(const Value& v, const std::string& where, Condition& c) {
    if (v.type != Type::Object) return fail(where, "must be an object");
    if (v.get("chance")) {
      c.type = Condition::Type::Chance;
      return object(v, where, {"chance"}) && integer(v, "chance", where, 2, 64, c.value);
    }
    const Value* counter = v.get("counter");
    if (!counter) return fail(where, "needs chance or counter");
    if (!object(v, where, {"counter", "atLeast", "atMost", "exactly"}) || v.members.size() != 2 ||
        !find(counter, where + ".counter", game.counters, "counter", c.counter)) {
      return fail(where, "needs a counter and one of atLeast, atMost, exactly");
    }
    const char* names[] = {"atLeast", "atMost", "exactly"};
    for (int i = 0; i < 3; ++i) {
      if (v.get(names[i])) {
        c.type = static_cast<Condition::Type>(i + 1);
        return integer(v, names[i], where, 0, kMaxCount, c.value);
      }
    }
    return fail(where, "needs one of atLeast, atMost, exactly");
  }

  static constexpr std::initializer_list<std::string_view> kWays = {
      "up", "right", "down", "left", "forward", "back", "same", "toward", "away", "random"};

  bool action(const Value& v, const std::string& where, Trigger::Type trigger, Action& a) {
    if (v.type != Type::Object || v.members.empty()) return fail(where, "must be an object");
    using T = Action::Type;
    static constexpr std::string_view verbs[] = {"move", "turn", "become", "vanish", "make", "count",
                                                 "say",  "sound", "go",   "shake", "win",  "lose"};
    const std::string& verb = v.members[0].first;
    int index = -1;
    for (int i = 0; i < 12; ++i) {
      if (v.get(verbs[i])) index = i;
    }
    if (index < 0) return fail(where, "unknown action \"" + verb + "\"");
    a.type = static_cast<T>(index);
    const Value& arg = *v.get(verbs[index]);
    std::string at = where + "." + std::string(verbs[index]);

    bool ok = true;
    switch (a.type) {
      case T::Move:
      case T::Turn:
        ok = object(v, where, {verbs[index], "who"}) && word(&arg, at, kWays, a.way);
        break;
      case T::Become:
        ok = object(v, where, {"become", "who"}) && kind(&arg, at, a.target);
        break;
      case T::Vanish:
        ok = object(v, where, {"vanish", "who"}) && must_be_true(arg, at);
        break;
      case T::Make: {
        const Value* to = field(v, "to", where, true);
        ok = object(v, where, {"make", "to", "who"}) && kind(&arg, at, a.target) && to &&
             word(to, where + ".to", kWays, a.way);
        break;
      }
      case T::Count:
        if (!object(v, where, {"count", "add", "set"}) || v.members.size() != 2 ||
            !find(&arg, at, game.counters, "counter", a.target)) {
          return fail(where, "needs a counter and one of add, set");
        }
        a.set = v.get("set") != nullptr;
        ok = a.set ? integer(v, "set", where, 0, kMaxCount, a.value)
                   : integer(v, "add", where, -kMaxCount, kMaxCount, a.value);
        break;
      case T::Say:
        ok = object(v, where, {"say"}) && text(v, "say", where, 1, kMaxText, a.text);
        break;
      case T::Win:
      case T::Lose:
        ok = object(v, where, {verbs[index]}) && text(v, verbs[index], where, 0, kMaxText, a.text);
        break;
      case T::Sound:
        ok = object(v, where, {"sound"}) && find(&arg, at, game.blips, "blip", a.target);
        break;
      case T::Go:
        ok = object(v, where, {"go"}) && find(&arg, at, game.screens, "screen", a.target);
        break;
      case T::Shake:
        ok = object(v, where, {"shake"}) && must_be_true(arg, at);
        break;
    }
    if (!ok) return false;

    if (const Value* who = v.get("who")) {
      if (!word(who, where + ".who", {"default", "you", "it", "bumper", "bumped"}, a.who)) return false;
      if (a.who == Who::Default) return fail(where + ".who", "must be one of: you, it, bumper, bumped");
      if (a.who == Who::It && trigger != Trigger::Type::Every) {
        return fail(where + ".who", "\"it\" is only for beat rules");
      }
      if ((a.who == Who::Bumper || a.who == Who::Bumped) && trigger != Trigger::Type::Bump) {
        return fail(where + ".who", "\"bumper\" and \"bumped\" are only for bump rules");
      }
    }
    return true;
  }

  bool rule(const Value& v, const std::string& where) {
    if (!object(v, where, {"when", "if", "do"})) return false;
    Rule r;
    const Value* when = field(v, "when", where, true);
    if (!when || !trigger(*when, where + ".when", r.when)) return false;
    if (const Value* conds = v.get("if")) {
      if (!array(conds, where + ".if", 0, kMaxConditions)) return false;
      r.conditions.resize(conds->items.size());
      for (size_t i = 0; i < r.conditions.size(); ++i) {
        if (!condition(conds->items[i], where + ".if[" + std::to_string(i) + "]", r.conditions[i])) return false;
      }
    }
    const Value* actions = field(v, "do", where, true);
    if (!actions || !array(actions, where + ".do", 1, kMaxActions)) return false;
    r.actions.resize(actions->items.size());
    for (size_t i = 0; i < r.actions.size(); ++i) {
      if (!action(actions->items[i], where + ".do[" + std::to_string(i) + "]", r.when.type, r.actions[i])) {
        return false;
      }
    }
    game.rules.push_back(std::move(r));
    return true;
  }

  template <typename F>
  bool list(const Value& root, std::string_view key, size_t min, size_t max, bool required, F each) {
    const Value* v = field(root, key, "game", required);
    if (!v) return !required;
    std::string where(key);
    if (!array(v, where, min, max)) return false;
    for (size_t i = 0; i < v->items.size(); ++i) {
      if (!each(v->items[i], where + "[" + std::to_string(i) + "]")) return false;
    }
    return true;
  }

  bool run(const Value& root) {
    if (!object(root, "game", {"format", "version", "title", "author", "palette", "tempo", "time", "seed",
                               "start", "counters", "things", "screens", "rules", "blips", "tunes"})) {
      return false;
    }
    const Value* format = field(root, "format", "game", true);
    if (!format) return false;
    if (format->type != Type::String || format->string != "chili-beat") {
      return fail("format", "this is not a Chili Beat game");
    }
    int version = 0;
    if (!integer(root, "version", "game", 1, 1000, version)) return false;
    if (version != 1) return fail("version", "this game needs a newer Chili Beat");

    if (!text(root, "title", "game", 1, 32, game.title) || !text(root, "author", "game", 0, 32, game.author, false) ||
        !integer(root, "tempo", "game", 2, 8, game.tempo)) {
      return false;
    }
    const Value* palette = field(root, "palette", "game", true);
    if (!palette || !array(palette, "palette", 4, 4)) return false;
    for (int i = 0; i < 4; ++i) {
      int64_t n;
      if (!integer(&palette->items[i], "palette[" + std::to_string(i) + "]", 0, 15, n)) return false;
      game.palette[i] = static_cast<uint8_t>(n);
    }
    const Value* time = field(root, "time", "game", true);
    int time_index = 0;
    if (!time || !word(time, "time", {"beat", "turns"}, time_index)) return false;
    game.turns = time_index == 1;
    if (const Value* seed = root.get("seed")) {
      int64_t n;
      if (!integer(seed, "seed", 0, 0xFFFFFFFFll, n)) return false;
      game.seed = static_cast<uint32_t>(n);
    }

    // Order matters: later parts refer to earlier ones.
    if (!list(root, "things", 1, kMaxThings, true, [&](auto& v, auto w) { return thing(v, w); }) ||
        !list(root, "counters", 0, kMaxCounters, false, [&](auto& v, auto w) { return counter(v, w); }) ||
        !list(root, "blips", 0, kMaxBlips, false, [&](auto& v, auto w) { return blip(v, w); }) ||
        !list(root, "tunes", 0, kMaxTunes, false, [&](auto& v, auto w) { return tune(v, w); }) ||
        !list(root, "screens", 1, kMaxScreens, true, [&](auto& v, auto w) { return screen(v, w); }) ||
        !unique(game.counters, "counters") || !unique(game.blips, "blips") || !unique(game.tunes, "tunes") ||
        !unique(game.screens, "screens") ||
        !list(root, "rules", 0, kMaxRules, true, [&](auto& v, auto w) { return rule(v, w); })) {
      return false;
    }
    const Value* start = field(root, "start", "game", true);
    return start && find(start, "start", game.screens, "screen", game.start);
  }
};

}  // namespace

bool load(std::string_view text, Game& game, std::string& error) {
  game = Game{};
  json::Value root;
  error = json::parse(text, root);
  if (!error.empty()) return false;
  Loader loader{game, {}};
  if (!loader.run(root)) {
    error = loader.error;
    game = Game{};
    return false;
  }
  return true;
}

}  // namespace chili_beat

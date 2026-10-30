// One test per trigger, condition and action, on small games built here.
#include <chili_beat/engine.hpp>

#include <string>

#include "check.hpp"
#include "helpers.hpp"

using namespace chili_beat;
using helpers::cell;
using helpers::find;
using helpers::play;

namespace {

// A 16 x 16 map with the given rows at the top, padded with empty cells.
std::string map(std::initializer_list<const char*> rows) {
  std::string out;
  int n = 0;
  for (const char* row : rows) {
    std::string padded = row;
    padded.resize(16, '.');
    out += std::string(n++ ? ",\n" : "") + "\"" + padded + "\"";
  }
  for (; n < 16; ++n) out += std::string(n ? ",\n" : "") + "\"................\"";
  return out;
}

const char* kPic = R"(["11111111","11111111","11111111","11111111","11111111","11111111","11111111","11111111"])";

// A game with things @ (you), g (you), #, c, o, k, r, counter n and blip ding.
// Screen "a" has the given rows; screens adds more screens.
Game mini_game(const std::string& rows, const std::string& rules, const std::string& top = "",
               const std::string& screens = "", const std::string& a_extra = "") {
  std::string things;
  for (const char* t : {"@", "g", "#", "c", "o", "k", "r"}) {
    bool you = t[0] == '@' || t[0] == 'g';
    things += std::string(things.empty() ? "" : ",") + R"({"key":")" + t + R"(","name":"T)" + t +
              R"(","you":)" + (you ? "true" : "false") + R"(,"frames":[)" + kPic + "]}";
  }
  std::string text = R"({"format":"chili-beat","version":1,"title":"Test","palette":[0,1,2,3],
    "tempo":4,"time":"beat","start":"a",)" + top + R"(
    "counters":[{"id":"n","name":"N","icon":"@"}],
    "blips":[{"id":"ding","preset":"coin"}],
    "things":[)" + things + R"(],
    "screens":[{"id":"a","map":[0,0],)" + a_extra + R"("things":[)" + rows + "]}" + screens + R"(],
    "rules":[)" + rules + "]}";
  Game game;
  std::string error;
  bool ok = load(text, game, error);
  if (!ok) std::cerr << "mini: " << error << "\n";
  CHECK(ok);
  return game;
}

Engine mini(const std::string& rows, const std::string& rules, const std::string& top = "",
            const std::string& screens = "", const std::string& a_extra = "") {
  return Engine(mini_game(rows, rules, top, screens, a_extra));
}

const std::string kArrowRule = R"({"when":{"press":"arrow"},"do":[{"move":"same"}]})";

char at(const Engine& e, int x, int y) {
  const auto& slot = e.board()[cell(x, y)];
  return slot ? e.game().things[slot->kind].key : '.';
}

int count(const Engine& e, char key) {
  int n = 0;
  for (const auto& slot : e.board()) n += slot && e.game().things[slot->kind].key == key;
  return n;
}

}  // namespace

// Triggers

TEST(trigger_start_runs_on_start) {
  Engine e = mini(map({"@"}), R"({"when":{"start":true},"do":[{"count":"n","add":2}]})");
  CHECK_EQ(e.counter(0), 2);
}

TEST(trigger_press_matches_buttons) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"count":"n","add":1}]},
    {"when":{"press":"b"},"do":[{"count":"n","add":10}]},
    {"when":{"press":"left"},"do":[{"count":"n","add":100}]},
    {"when":{"press":"arrow"},"do":[{"count":"n","add":200}]})");
  play(e, "a b up left");
  CHECK_EQ(e.counter(0), 1 + 10 + 200 + 300);
}

TEST(trigger_bump_pick_up_moves_in) {
  Engine e = mini(map({"@k"}), kArrowRule + R"(,{"when":{"bump":["you","k"]},
    "do":[{"vanish":true,"who":"bumped"},{"count":"n","add":1}]})");
  play(e, "right");
  CHECK_EQ(find(e, '@'), cell(1, 0));
  CHECK_EQ(count(e, 'k'), 0);
  CHECK_EQ(e.counter(0), 1);
}

TEST(trigger_bump_push_chains) {
  Engine e = mini(map({"............@cc."}), kArrowRule + R"(,{"when":{"bump":["any","c"]},"do":[{"move":"same","who":"bumped"}]})");
  play(e, "right");
  CHECK_EQ(find(e, '@'), cell(13, 0));
  CHECK(e.board()[cell(14, 0)] && e.board()[cell(15, 0)]);
  play(e, "right");  // the last crate is at the edge now
  CHECK_EQ(find(e, '@'), cell(13, 0));
}

TEST(trigger_bump_you_and_any) {
  Engine e = mini(map({"@#"}), kArrowRule + R"(,{"when":{"bump":["you","any"]},"do":[{"count":"n","add":1}]})");
  play(e, "right right");
  CHECK_EQ(e.counter(0), 2);
  CHECK_EQ(find(e, '@'), cell(0, 0));
}

TEST(bump_chains_stop_after_16) {
  // Each bump makes the other thing push back: it never ends on its own.
  Engine e = mini(map({".@c"}), kArrowRule + R"(,
    {"when":{"bump":["@","c"]},"do":[{"move":"left","who":"bumped"},{"count":"n","add":1}]},
    {"when":{"bump":["c","@"]},"do":[{"move":"right","who":"bumped"},{"count":"n","add":1}]})");
  play(e, "right");
  CHECK_EQ(e.counter(0), 16);
  CHECK_EQ(find(e, '@'), cell(1, 0));
  CHECK_EQ(find(e, 'c'), cell(2, 0));
}

TEST(trigger_every_n_beats) {
  Engine e = mini(map({"@r.r"}), R"({"when":{"every":2,"thing":"r"},"do":[{"count":"n","add":1}]})");
  play(e, "4 beat");
  CHECK_EQ(e.counter(0), 4);  // 2 robots, beats 2 and 4
}

TEST(trigger_counter_reaches) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"count":"n","add":1}]},
    {"when":{"counter":"n","reaches":2},"do":[{"say":"two"}]})");
  play(e, "a");
  CHECK_EQ(e.text(), "");
  play(e, "a");
  CHECK_EQ(e.text(), "two");
}

// Conditions

TEST(condition_counter_compares) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"count":"n","add":1}]},
    {"when":{"press":"b"},"if":[{"counter":"n","atLeast":2}],"do":[{"say":"least"}]},
    {"when":{"press":"b"},"if":[{"counter":"n","atMost":0}],"do":[{"say":"most"}]},
    {"when":{"press":"b"},"if":[{"counter":"n","exactly":1}],"do":[{"say":"exactly"}]})");
  play(e, "b");
  CHECK_EQ(e.text(), "most");
  play(e, "a a b");
  CHECK_EQ(e.text(), "exactly");
  play(e, "a a b");
  CHECK_EQ(e.text(), "least");
}

TEST(condition_chance_is_seeded) {
  auto run = [](int seed) {
    Engine e = mini(map({"@"}), R"({"when":{"every":1,"thing":"@"},"if":[{"chance":4}],"do":[{"count":"n","add":1}]})",
                    "\"seed\":" + std::to_string(seed) + ",");
    play(e, "200 beat");
    return e.counter(0);
  };
  CHECK(run(1) > 30 && run(1) < 70);
  CHECK_EQ(run(1), run(1));
  CHECK(run(1) != run(2) || run(1) != run(3));
}

// Actions

TEST(action_move_ways) {
  Engine e = mini(map({"", "", "....r.......@..."}), R"(
    {"when":{"press":"a"},"do":[{"move":"toward","who":"you"}]},
    {"when":{"press":"b"},"do":[{"move":"down"},{"move":"back"},{"move":"forward"},{"move":"forward"}]})");
  play(e, "a");  // the only you thing has no other you to go toward
  CHECK_EQ(find(e, '@'), cell(12, 2));
  play(e, "b");  // down, back (up), forward (up), forward (up)
  CHECK_EQ(find(e, '@'), cell(12, 0));
}

TEST(action_move_toward_and_away) {
  Engine e = mini(map({"", "", "....r.......@..."}), R"(
    {"when":{"press":"a"},"do":[{"move":"toward","who":"you"}]},
    {"when":{"every":1,"thing":"r"},"do":[{"move":"toward"}]},
    {"when":{"press":"b"},"do":[{"move":"away","who":"you"}]})");
  play(e, "beat");
  CHECK_EQ(find(e, 'r'), cell(5, 2));
  play(e, "beat");
  CHECK_EQ(find(e, 'r'), cell(6, 2));

  Engine away = mini(map({"....g...@"}), R"({"when":{"press":"a"},"do":[{"move":"away","who":"you"}]})");
  play(away, "a");
  CHECK_EQ(find(away, 'g'), cell(3, 0));
  CHECK_EQ(find(away, '@'), cell(9, 0));
}

TEST(action_move_random_stays_on_board) {
  Engine e = mini(map({"@"}), R"({"when":{"every":1,"thing":"@"},"do":[{"move":"random"}]})");
  play(e, "50 beat");
  CHECK(find(e, '@') >= 0);
  CHECK(find(e, '@') != cell(0, 0));
}

TEST(each_thing_moves_once_per_beat) {
  Engine e = mini(map({"r..."}), R"({"when":{"every":1,"thing":"r"},"do":[{"move":"right"},{"move":"right"}]})");
  play(e, "beat");
  CHECK_EQ(find(e, 'r'), cell(1, 0));
}

TEST(action_turn_changes_forward) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"turn":"right"},{"move":"forward"}]})");
  play(e, "a");
  CHECK_EQ(find(e, '@'), cell(1, 0));
}

TEST(action_become_keeps_place_and_way) {
  Engine e = mini(map({"", "@"}), kArrowRule + R"(,{"when":{"press":"b"},"do":[{"become":"g"}]},
    {"when":{"press":"a"},"do":[{"move":"forward"}]})");
  play(e, "right b a");
  CHECK_EQ(find(e, 'g'), cell(2, 1));
  CHECK_EQ(find(e, '@'), -1);
}

TEST(action_make_only_into_empty_cells) {
  Engine e = mini(map({"@#"}), R"({"when":{"press":"a"},"do":[{"make":"k","to":"right"},{"make":"k","to":"down"}]})");
  play(e, "a");
  CHECK_EQ(count(e, 'k'), 1);
  CHECK_EQ(find(e, 'k'), cell(0, 1));
}

TEST(made_things_wait_for_the_next_beat) {
  Engine e = mini(map({"r..."}), R"({"when":{"every":1,"thing":"r"},"do":[{"make":"r","to":"right"}]})");
  play(e, "beat");
  CHECK_EQ(count(e, 'r'), 2);
  play(e, "beat");
  CHECK_EQ(count(e, 'r'), 3);
}

TEST(action_count_stays_in_range) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"count":"n","add":-5}]},
    {"when":{"press":"b"},"do":[{"count":"n","set":998},{"count":"n","add":5}]})");
  play(e, "a");
  CHECK_EQ(e.counter(0), 0);
  play(e, "b");
  CHECK_EQ(e.counter(0), 999);
}

TEST(action_say_pages_and_pauses) {
  Engine e = mini(map({"@"}), kArrowRule + R"(,{"when":{"press":"a"},
    "do":[{"say":"one two three four five six seven eight nine ten eleven twelve"}]})");
  play(e, "a");
  CHECK_EQ(e.text(), "one two three four\nfive six seven eight\nnine ten eleven");
  uint32_t beat = e.beat();
  play(e, "right 40 wait");
  CHECK_EQ(find(e, '@'), cell(0, 0));  // paused
  CHECK_EQ(e.beat(), beat);
  play(e, "a");
  CHECK_EQ(e.text(), "twelve");
  play(e, "a right");
  CHECK_EQ(e.text(), "");
  CHECK_EQ(find(e, '@'), cell(1, 0));
}

TEST(action_sound_reports_the_blip) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"sound":"ding"}]})");
  e.step(kA);
  CHECK_EQ(e.sounds().size(), 1u);
  CHECK_EQ(e.fired().size(), 1u);
  e.step(0);
  CHECK_EQ(e.sounds().size(), 0u);
}

TEST(action_go_places_you_from_the_map) {
  Engine e = mini(map({"@"}), R"({"when":{"press":"a"},"do":[{"go":"b"}]},
    {"when":{"press":"b"},"do":[{"go":"a"}]})", "",
    R"(,{"id":"b","map":[3,3],"things":[)" + map({"", "", "..g"}) + "]}");
  play(e, "a");
  CHECK_EQ(e.screen(), 1);
  CHECK_EQ(find(e, 'g'), cell(2, 2));
  CHECK_EQ(find(e, '@'), -1);
  play(e, "b");
  CHECK_EQ(e.screen(), 0);
  CHECK_EQ(find(e, '@'), cell(0, 0));
}

TEST(screens_keep_state_unless_reset) {
  std::string rules = kArrowRule + R"(,{"when":{"press":"a"},"do":[{"go":"b"}]},
    {"when":{"press":"b"},"do":[{"go":"a"}]},
    {"when":{"bump":["any","k"]},"do":[{"vanish":true,"who":"bumped"}]})";
  std::string b = R"(,{"id":"b","map":[1,0],"things":[)" + map({"g"}) + "]}";
  Engine kept = mini(map({"@k"}), rules, "", b);
  play(kept, "right a b");
  CHECK_EQ(count(kept, 'k'), 0);
  CHECK_EQ(find(kept, '@'), cell(0, 0));  // go places you from the map
  Engine reset = mini(map({"@k"}), rules, "", b, R"("reset":true,)");
  play(reset, "right a b");
  CHECK_EQ(count(reset, 'k'), 1);
}

TEST(action_shake_lasts_one_beat) {
  Engine e = mini(map({"", "", "@"}), R"({"when":{"press":"a"},"do":[{"shake":true}]})");
  Frame still, shaking;
  e.draw(still);
  e.step(kA);
  e.step(0);
  e.draw(shaking);
  CHECK(still.pixels != shaking.pixels);
  for (int i = 0; i < 15; ++i) e.step(0);
  e.draw(shaking);
  CHECK(still.pixels == shaking.pixels);
}

TEST(action_win_and_lose_restart_on_a) {
  Engine e = mini(map({"@"}), kArrowRule + R"(,{"when":{"press":"a"},"do":[{"count":"n","add":1},{"win":""},{"lose":"no"},{"count":"n","add":1}]},
    {"when":{"press":"a"},"do":[{"count":"n","add":10}]})");
  play(e, "right a");
  CHECK(e.state() == Engine::State::Won);
  CHECK_EQ(e.text(), "You win!");
  CHECK_EQ(e.counter(0), 2);  // the rest of the card runs, the next card does not
  play(e, "a");
  CHECK(e.state() == Engine::State::Playing);
  CHECK_EQ(e.counter(0), 0);
  CHECK_EQ(find(e, '@'), cell(0, 0));
}

TEST(who_defaults_and_bumped) {
  Engine e = mini(map({"@c"}), kArrowRule + R"(,{"when":{"bump":["@","c"]},"do":[{"become":"k","who":"bumped"},{"become":"g"}]})");
  play(e, "right");
  CHECK_EQ(find(e, 'g'), cell(0, 0));
  CHECK_EQ(find(e, 'k'), cell(1, 0));
}

TEST(turns_mode_runs_beat_rules_after_each_press) {
  Game game = mini_game(map({"@", "", "r"}), R"({"when":{"every":1,"thing":"r"},"do":[{"move":"right"}]})");
  game.turns = true;
  Engine e(std::move(game));
  play(e, "60 wait");
  CHECK_EQ(find(e, 'r'), cell(0, 2));
  play(e, "a b");
  CHECK_EQ(find(e, 'r'), cell(2, 2));
}

// Whole games

TEST(bee_home_replays_to_a_win) {
  Engine e(helpers::fixture("bee-home.beat"));
  play(e, "4 up left");
  CHECK_EQ(e.counter(0), 1);
  CHECK_EQ(find(e, 'u'), cell(2, 2));
  for (int i = 0; i < 4; ++i) play(e, "8 beat left");
  CHECK_EQ(e.counter(0), 5);
  play(e, "down down right");
  CHECK(e.state() == Engine::State::Won);
  CHECK_EQ(e.text(), "The hive is happy.");
  play(e, "a");
  CHECK(e.state() == Engine::State::Playing);
  CHECK_EQ(e.counter(0), 0);
}

TEST(bee_home_hive_asks_for_pollen) {
  Engine e(helpers::fixture("bee-home.beat"));
  play(e, "2 up right");
  CHECK_EQ(e.text(), "Bring me 5 pollen.");
}

TEST(every_action_survives_a_long_random_play) {
  Engine e(helpers::fixture("every-action.beat"));
  uint32_t seed = 12345;
  for (int i = 0; i < 20000; ++i) {
    seed = seed * 1103515245 + 12345;
    e.step(static_cast<uint8_t>(seed >> 26));
    Frame f;
    if (i % 100 == 0) e.draw(f);
  }
  CHECK(e.steps() == 20000u);
}

TEST(crate_puzzle_push_into_hole) {
  Engine e(helpers::fixture("crate-puzzle.beat"));
  CHECK_EQ(e.counter(0), 3);
  play(e, "up 5 right");
  CHECK_EQ(at(e, 10, 5), 'c');
  play(e, "right");
  CHECK_EQ(e.counter(0), 2);
  CHECK_EQ(find(e, 's'), cell(11, 5));
  CHECK_EQ(find(e, '@'), cell(10, 5));
}

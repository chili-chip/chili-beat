#include <chili_beat/game.hpp>

#include <string>

#include "check.hpp"
#include "helpers.hpp"

using namespace chili_beat;

TEST(load_all_fixtures) {
  for (auto name : {"blank.beat", "bee-home.beat", "crate-puzzle.beat", "garden-toy.beat", "every-action.beat"}) {
    helpers::fixture(name);
  }
  Game g = helpers::fixture("bee-home.beat");
  CHECK_EQ(g.title, "Bee Home");
  CHECK_EQ(g.tempo, 4);
  CHECK_EQ(int(g.palette[2]), 4);
  CHECK_EQ(g.things.size(), 7u);
  CHECK_EQ(g.rules.size(), 7u);
  CHECK(g.rules[1].when.type == Trigger::Type::Bump);
  CHECK_EQ(int(g.rules[1].actions[0].target), 2);  // Bud
  CHECK(g.rules[1].actions[0].who == Who::Bumped);
  CHECK(g.rules[5].conditions[0].type == Condition::Type::AtLeast);
  CHECK_EQ(int(g.screens[0].things[helpers::cell(3, 6)]), 0);  // Bee
  CHECK_EQ(int(g.screens[0].tune), 0);
  CHECK(helpers::fixture("crate-puzzle.beat").turns);
}

// Loads bee-home.beat with one text replacement and returns the error.
static std::string error_after(const std::string& from, const std::string& to) {
  std::string text = helpers::read("bee-home.beat");
  size_t at = text.find(from);
  CHECK(at != std::string::npos);
  text.replace(at, from.size(), to);
  Game game;
  std::string error;
  CHECK(!load(text, game, error));
  return error;
}

TEST(load_gives_clear_errors) {
  Game game;
  std::string error;
  CHECK(!load("not json", game, error));
  CHECK(!load(R"({"format": "other"})", game, error));
  CHECK_EQ(error, "format: this is not a Chili Beat game");

  CHECK_EQ(error_after("\"version\": 1", "\"version\": 2"), "version: this game needs a newer Chili Beat");
  CHECK_EQ(error_after("\"tempo\": 4", "\"tempo\": 9"), "game.tempo: must be from 2 to 8");
  CHECK_EQ(error_after("\"tempo\": 4", "\"tempo\": 4, \"colour\": 1"), "game: unknown field \"colour\"");
  CHECK_EQ(error_after("\"become\": \"u\"", "\"become\": \"z\""), "rules[1].do[0].become: no thing has key \"z\"");
  CHECK_EQ(error_after("\"sound\": \"coin\"", "\"sound\": \"ding\""), "rules[1].do[2].sound: no blip has id \"ding\"");
  CHECK_EQ(error_after("\"who\": \"bumped\"", "\"who\": \"it\""), "rules[1].do[0].who: \"it\" is only for beat rules");
  CHECK_EQ(error_after("\"start\": \"meadow\"", "\"start\": \"cave\""), "start: no screen has id \"cave\"");
  CHECK_EQ(error_after("\"key\": \"u\"", "\"key\": \"f\""), "things[2].key: key \"f\" is used twice");
  CHECK_EQ(error_after("#...h...#..w...#", "#...h...#..Q...#"), "screens[0].things[4]: no thing has key \"Q\"");
  CHECK_EQ(error_after("\"atLeast\": 5", "\"atLeast\": 5, \"atMost\": 6"),
           "rules[5].if[0]: needs a counter and one of atLeast, atMost, exactly");
}

#include <chili_beat/engine.hpp>

#include "check.hpp"
#include "helpers.hpp"

using namespace chili_beat;
using helpers::cell;
using helpers::find;
using helpers::play;

TEST(arrows_move_you) {
  Engine e(helpers::fixture("blank.beat"));
  CHECK_EQ(find(e, '@'), cell(7, 7));
  play(e, "right");
  CHECK_EQ(find(e, '@'), cell(8, 7));
  play(e, "up up left");
  CHECK_EQ(find(e, '@'), cell(7, 5));
}

TEST(full_cell_with_no_rule_is_a_wall) {
  Engine e(helpers::fixture("blank.beat"));
  play(e, "10 left");
  CHECK_EQ(find(e, '@'), cell(1, 7));
  CHECK_EQ(find(e, '#'), cell(0, 0));
}

TEST(walk_off_the_edge_to_the_next_screen) {
  Engine e(helpers::fixture("every-action.beat"));
  CHECK_EQ(e.screen(), 0);
  play(e, "up 8 right");
  CHECK_EQ(find(e, '@'), cell(15, 7));
  play(e, "right");
  CHECK_EQ(e.screen(), 1);
  CHECK_EQ(find(e, '@'), cell(0, 7));
  play(e, "left");  // the way back is a wall on screen one
  CHECK_EQ(e.screen(), 0);
  CHECK_EQ(find(e, '@'), cell(15, 7));
}

TEST(turns_mode_beats_once_per_press) {
  Engine e(helpers::fixture("crate-puzzle.beat"));
  play(e, "60 wait");
  CHECK_EQ(e.beat(), 0u);
  play(e, "up down");
  CHECK_EQ(e.beat(), 2u);
}

TEST(beat_mode_beats_on_the_tempo) {
  Engine e(helpers::fixture("blank.beat"));  // tempo 4
  for (int i = 0; i < 60; ++i) e.step(0);
  CHECK_EQ(e.beat(), 4u);
}

TEST(update_steps_at_60_hz_and_presents) {
  Engine e(helpers::fixture("blank.beat"));
  helpers::MockHost host;
  host.delta = 1000;
  e.update(host);
  CHECK_EQ(e.steps(), 15u);  // a long gap catches up at most a quarter second
  host.delta = 50;
  e.update(host);
  CHECK_EQ(e.steps(), 18u);
  CHECK_EQ(host.frames.size(), 2u);
}

static int pixel(const Frame& f, int x, int y) { return f.pixels[y * kScreenPixels + x]; }

TEST(draws_things_slides_and_frames) {
  Engine e(helpers::fixture("blank.beat"));
  Frame f;
  e.draw(f);
  // Hero frame 0 at (7, 7): top row "00333300".
  CHECK_EQ(pixel(f, 56 + 1, 56), 0);
  CHECK_EQ(pixel(f, 56 + 2, 56), 3);
  // Wall at (0, 0): "22222222".
  CHECK_EQ(pixel(f, 0, 0), 2);

  // Frames flip on the clock beat: bottom row "00300300" then "03000030".
  for (int i = 0; i < 15; ++i) e.step(0);
  e.draw(f);
  CHECK_EQ(pixel(f, 56 + 1, 63), 3);

  // Mid-slide, the hero is between cells. Tempo 4: a slide takes 3.75 steps.
  e.step(kRight);
  e.draw(f);
  CHECK_EQ(pixel(f, 64 + 2, 56), 0);
  for (int i = 0; i < 4; ++i) e.step(0);
  e.draw(f);
  CHECK_EQ(pixel(f, 64 + 2, 56), 3);
}

TEST(draws_counters_top_left) {
  Engine e(helpers::fixture("bee-home.beat"));
  Frame f;
  e.draw(f);
  // Flower icon at (0, 0): row 1 "00202000".
  CHECK_EQ(pixel(f, 2, 1), 2);
  // "0" at x = 9: row 0 ".###.".
  CHECK_EQ(pixel(f, 9, 0), 0);
  CHECK_EQ(pixel(f, 10, 0), 3);
}

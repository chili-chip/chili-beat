#include <chili_beat/engine.hpp>

#include "check.hpp"
#include "helpers.hpp"
#include "sound.hpp"

using namespace chili_beat;
using helpers::play;

TEST(notes_follow_the_scale) {
  using S = Tune::Scale;
  CHECK_EQ(sound::note_hz(S::Major, 1, 262), 262);
  CHECK_EQ(sound::note_hz(S::Major, 5, 262), 392);   // G
  CHECK_EQ(sound::note_hz(S::Major, 8, 262), 524);   // C one octave up
  CHECK_EQ(sound::note_hz(S::Minor, 3, 262), 311);   // E flat
  CHECK_EQ(sound::note_hz(S::PentatonicMajor, 6, 262), 524);
  CHECK_EQ(sound::note_hz(S::PentatonicMinor, 15, 262), 1864);  // B flat, two octaves up
}

// Bee Home: tempo 4 (a beat is 15 steps), pentatonic major,
// lead 1 0 3 0 5 …, beat 8 0 1 0 9 ….
TEST(tune_plays_one_step_per_beat) {
  Engine e(helpers::fixture("bee-home.beat"));
  Frame f;
  e.mix(f);
  CHECK_EQ(f.channels[0].frequency, 262);  // lead 1 starts at once
  CHECK_EQ(f.channels[0].volume, 140);
  CHECK(f.channels[1].wave == Channel::Wave::Triangle);  // kick
  CHECK_EQ(f.channels[1].volume, 255);

  for (int i = 0; i < 14; ++i) e.step(0);
  e.mix(f);
  CHECK_EQ(f.channels[0].frequency, 262);  // fading out at the end of the beat
  CHECK(f.channels[0].volume > 0 && f.channels[0].volume < 20);
  CHECK_EQ(f.channels[1].volume, 0);  // the kick is short

  e.step(0);  // beat 2: rests
  e.step(0);
  for (int i = 0; i < 14; ++i) e.step(0);  // beat 3: lead 3, bass 1
  e.mix(f);
  CHECK_EQ(f.channels[0].frequency, 330);
  CHECK_EQ(f.channels[1].frequency, 131);
  CHECK(f.channels[1].wave == Channel::Wave::Triangle);

  for (int i = 0; i < 15; ++i) e.step(0);
  for (int i = 0; i < 15; ++i) e.step(0);  // beat 5: lead 5, snare
  e.mix(f);
  CHECK_EQ(f.channels[0].frequency, 440);
  CHECK(f.channels[1].wave == Channel::Wave::Noise);
}

TEST(blip_plays_on_a_free_channel) {
  Engine e(helpers::fixture("bee-home.beat"));
  play(e, "4 up");
  e.step(kLeft);  // bump the flower: play coin (pitch 5, length 2, slide 3)
  CHECK_EQ(e.sounds().size(), 1u);
  Frame f;
  e.mix(f);
  CHECK(f.channels[2].wave == Channel::Wave::Square);
  CHECK_EQ(f.channels[2].frequency, 1111);  // 988 * 9 / 8
  CHECK_EQ(f.channels[2].volume, 200);
  CHECK_EQ(f.channels[3].volume, 0);
  for (int i = 0; i < 6; ++i) e.step(0);
  e.mix(f);
  CHECK(f.channels[2].frequency > 1111 * 4 / 3);  // the coin's jump up, plus slide
  for (int i = 0; i < 4; ++i) e.step(0);
  e.mix(f);
  CHECK_EQ(f.channels[2].volume, 0);  // 12 * 4 / 5 = 9 steps long
}

TEST(blip_presets_all_end) {
  for (int p = 0; p < 7; ++p) {
    Blip b;
    b.preset = static_cast<Blip::Preset>(p);
    b.length = 7;
    b.slide = 7;
    b.pitch = 7;
    CHECK(sound::blip(b, 0).volume > 0);
    CHECK_EQ(sound::blip(b, 200).volume, 0);
  }
}

TEST(no_tune_is_silent) {
  Engine e(helpers::fixture("blank.beat"));
  for (int i = 0; i < 40; ++i) e.step(0);
  Frame f;
  e.mix(f);
  for (const auto& c : f.channels) CHECK_EQ(c.volume, 0);
}

#include "sound.hpp"

#include <algorithm>

namespace chili_beat::sound {
namespace {

constexpr int kLeadRoot = 262;  // C4
constexpr int kBassRoot = 131;  // C3

// 2^(n/12) * 1000 for one octave of semitones.
constexpr int kSemitone[12] = {1000, 1059, 1122, 1189, 1260, 1335, 1414, 1498, 1587, 1682, 1782, 1888};

struct ScaleSteps {
  int size;
  int steps[7];
};

constexpr ScaleSteps kScales[] = {
    {7, {0, 2, 4, 5, 7, 9, 11}},  // major
    {7, {0, 2, 3, 5, 7, 8, 10}},  // minor
    {5, {0, 2, 4, 7, 9}},         // pentatonic major
    {5, {0, 3, 5, 7, 10}},        // pentatonic minor
};

// Volume falling in a straight line from `from` to 0 over `steps`.
uint8_t fade(int from, uint32_t elapsed, int steps) {
  if (steps <= 0 || elapsed >= static_cast<uint32_t>(steps)) return 0;
  return static_cast<uint8_t>(from * (steps - static_cast<int>(elapsed)) / steps);
}

struct Preset {
  Channel::Wave wave;
  int hz;       // at pitch 4
  int steps;    // at length 3
  int slide;    // +1 up, -1 down
};

constexpr Preset kPresets[] = {
    {Channel::Wave::Square, 988, 12, 1},     // coin: a jump up halfway
    {Channel::Wave::Square, 300, 14, 1},     // jump
    {Channel::Wave::Noise, 1600, 8, -1},     // hit
    {Channel::Wave::Noise, 400, 30, -1},     // boom
    {Channel::Wave::Triangle, 220, 4, -1},   // step
    {Channel::Wave::Square, 220, 36, 1},     // power
    {Channel::Wave::Square, 880, 5, -1},     // blip
};

}  // namespace

uint16_t note_hz(Tune::Scale scale, int degree, int root_hz) {
  const ScaleSteps& s = kScales[static_cast<int>(scale)];
  int index = degree - 1;
  int semitones = s.steps[index % s.size] + 12 * (index / s.size);
  return static_cast<uint16_t>((root_hz * kSemitone[semitones % 12] / 1000) << (semitones / 12));
}

Channel lead(Tune::Scale scale, int note, uint32_t elapsed, int beat_steps) {
  Channel c;
  if (note == 0) return c;
  c.wave = Channel::Wave::Square;
  c.frequency = note_hz(scale, note, kLeadRoot);
  c.volume = fade(140, elapsed, beat_steps);
  return c;
}

Channel beat(Tune::Scale scale, int note, uint32_t elapsed, int beat_steps) {
  Channel c;
  switch (note) {
    case 0:
      break;
    case 8:  // kick: a quick drop in pitch
      c.wave = Channel::Wave::Triangle;
      c.frequency = static_cast<uint16_t>(std::max(50, 160 - static_cast<int>(elapsed) * 20));
      c.volume = fade(255, elapsed, 6);
      break;
    case 9:  // snare
      c.wave = Channel::Wave::Noise;
      c.frequency = 3000;
      c.volume = fade(170, elapsed, 6);
      break;
    case 10:  // hi-hat
      c.wave = Channel::Wave::Noise;
      c.frequency = 9000;
      c.volume = fade(90, elapsed, 3);
      break;
    default:  // bass
      c.wave = Channel::Wave::Triangle;
      c.frequency = note_hz(scale, note, kBassRoot);
      c.volume = fade(220, elapsed, beat_steps);
      break;
  }
  return c;
}

Channel blip(const Blip& blip, uint32_t elapsed) {
  const Preset& p = kPresets[static_cast<int>(blip.preset)];
  int steps = p.steps * (2 + blip.length) / 5;
  Channel c;
  if (elapsed >= static_cast<uint32_t>(steps)) return c;
  int t = static_cast<int>(elapsed);
  int hz = p.hz * (4 + blip.pitch) / 8;
  if (blip.preset == Blip::Preset::Coin && t * 2 >= steps) hz = hz * 4 / 3;
  // Slide: up to the full base frequency over the whole blip.
  hz += p.slide * hz * blip.slide * t / (8 * steps);
  c.wave = p.wave;
  c.frequency = static_cast<uint16_t>(std::clamp(hz, 20, 20000));
  if (blip.preset == Blip::Preset::Power) {  // holds, then fades in the second half
    c.volume = t * 2 < steps ? 200 : fade(200, elapsed - steps / 2, steps - steps / 2);
  } else {
    c.volume = fade(200, elapsed, steps);
  }
  return c;
}

}  // namespace chili_beat::sound

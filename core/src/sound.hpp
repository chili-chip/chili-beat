// Turns tune notes and blips into channel states. See docs/sound.md.
#pragma once

#include <chili_beat/game.hpp>
#include <chili_beat/host.hpp>

#include <cstdint>

namespace chili_beat::sound {

// Hz of a scale degree (1 is the root) above a root note.
uint16_t note_hz(Tune::Scale scale, int degree, int root_hz);

// Lead and beat voices, `elapsed` steps after the note started.
Channel lead(Tune::Scale scale, int note, uint32_t elapsed, int beat_steps);
Channel beat(Tune::Scale scale, int note, uint32_t elapsed, int beat_steps);

// A blip, `elapsed` steps after it started. Volume 0 when it has ended.
Channel blip(const Blip& blip, uint32_t elapsed);

}  // namespace chili_beat::sound

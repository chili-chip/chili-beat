// What a player (web, console, tests) gives the engine and gets back.
#pragma once

#include <array>
#include <cstdint>

#include "game.hpp"

namespace chili_beat {

// One sound channel for one step. The host sets its synth to this every frame.
struct Channel {
  enum class Wave : uint8_t { Square, Triangle, Noise };
  Wave wave = Wave::Square;
  uint16_t frequency = 0;  // Hz
  uint8_t volume = 0;      // 0 is silent, 255 is full
};

// Channels: 0 tune lead, 1 tune beat (bass and drums), 2 and 3 blips.
constexpr int kChannels = 4;

// What the host shows and plays for one step.
struct Frame {
  // Colour numbers 0-3, row by row. Look them up in Game::palette, then kSweetie16.
  std::array<uint8_t, kScreenPixels * kScreenPixels> pixels{};
  std::array<Channel, kChannels> channels{};
};

class Host {
 public:
  virtual ~Host() = default;
  // Milliseconds since the last call.
  virtual uint32_t delta_time_ms() = 0;
  // Buttons held now, as a Button mask.
  virtual uint8_t buttons() = 0;
  // Shows a new frame.
  virtual void present(const Frame& frame) = 0;
};

}  // namespace chili_beat

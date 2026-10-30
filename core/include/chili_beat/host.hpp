// What a player (web, console, tests) gives the engine and gets back.
#pragma once

#include <array>
#include <cstdint>

#include "game.hpp"

namespace chili_beat {

// One drawn picture: colour numbers 0-3. Look them up in Game::palette, then kSweetie16.
struct Frame {
  std::array<uint8_t, kScreenPixels * kScreenPixels> pixels{};
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

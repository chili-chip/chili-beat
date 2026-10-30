// The Chili Beat 6 × 8 font: printable ASCII, 5 pixels wide plus 1 space.
#pragma once

#include <cstdint>

namespace chili_beat::font {

constexpr int kWidth = 6;
constexpr int kHeight = 8;

// Row bits for one character, top row first. Bit 4 is the left pixel.
// Characters outside printable ASCII draw as '?'.
const uint8_t* glyph(char c);

}  // namespace chili_beat::font

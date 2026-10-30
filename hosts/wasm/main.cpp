// The web player's C API. See docs/web.md.
#include <chili_beat/engine.hpp>

#include <emscripten/emscripten.h>

#include <array>
#include <cstdio>
#include <optional>
#include <string>

using namespace chili_beat;

namespace {

std::optional<Engine> g_engine;
std::string g_error;
std::string g_debug;
std::array<uint8_t, kScreenPixels * kScreenPixels * 4> g_rgba{};
std::array<uint8_t, kChannels * 4> g_sound{};

void append_json_string(std::string& out, const std::string& s) {
  out += '"';
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += static_cast<char>(c);
    } else if (c < 0x20) {
      char buf[8];
      std::snprintf(buf, sizeof buf, "\\u%04x", c);
      out += buf;
    } else {
      out += static_cast<char>(c);
    }
  }
  out += '"';
}

}  // namespace

extern "C" {

// Loads a game from .beat text. Returns 1, or 0 and sets cb_error().
EMSCRIPTEN_KEEPALIVE int cb_load(const char* text) {
  Game game;
  if (!load(text, game, g_error)) {
    g_engine.reset();
    return 0;
  }
  g_error.clear();
  g_engine.emplace(std::move(game));
  return 1;
}

EMSCRIPTEN_KEEPALIVE const char* cb_error() { return g_error.c_str(); }

// One step (1/60 s). buttons: up 1, down 2, left 4, right 8, A 16, B 32.
EMSCRIPTEN_KEEPALIVE void cb_step(int buttons) {
  if (g_engine) g_engine->step(static_cast<uint8_t>(buttons));
}

EMSCRIPTEN_KEEPALIVE void cb_restart() {
  if (g_engine) g_engine->restart();
}

// The screen now, as 128 x 128 RGBA.
EMSCRIPTEN_KEEPALIVE const uint8_t* cb_rgba() {
  if (!g_engine) return g_rgba.data();
  Frame frame;
  g_engine->draw(frame);
  const auto& palette = g_engine->game().palette;
  for (int i = 0; i < kScreenPixels * kScreenPixels; ++i) {
    uint32_t c = kSweetie16[palette[frame.pixels[i]]];
    g_rgba[i * 4 + 0] = c >> 16;
    g_rgba[i * 4 + 1] = (c >> 8) & 0xFF;
    g_rgba[i * 4 + 2] = c & 0xFF;
    g_rgba[i * 4 + 3] = 255;
  }
  return g_rgba.data();
}

// The 4 sound channels now: wave (0 square, 1 triangle, 2 noise), volume,
// frequency low byte, frequency high byte.
EMSCRIPTEN_KEEPALIVE const uint8_t* cb_sound() {
  g_sound.fill(0);
  if (!g_engine) return g_sound.data();
  Frame frame;
  g_engine->mix(frame);
  for (int i = 0; i < kChannels; ++i) {
    const Channel& c = frame.channels[i];
    g_sound[i * 4 + 0] = static_cast<uint8_t>(c.wave);
    g_sound[i * 4 + 1] = c.volume;
    g_sound[i * 4 + 2] = c.frequency & 0xFF;
    g_sound[i * 4 + 3] = c.frequency >> 8;
  }
  return g_sound.data();
}

// State as JSON: state, screen, beat, counters, fired (rules that ran in the
// last step), sounds (blips started in the last step) and text.
EMSCRIPTEN_KEEPALIVE const char* cb_debug() {
  g_debug.clear();
  if (!g_engine) return "null";
  const Engine& e = *g_engine;
  const char* states[] = {"playing", "won", "lost"};
  g_debug += "{\"state\":\"";
  g_debug += states[static_cast<int>(e.state())];
  g_debug += "\",\"screen\":";
  append_json_string(g_debug, e.game().screens[e.screen()].id);
  g_debug += ",\"beat\":" + std::to_string(e.beat()) + ",\"counters\":[";
  for (size_t i = 0; i < e.game().counters.size(); ++i) {
    g_debug += (i ? "," : "") + std::to_string(e.counter(static_cast<int>(i)));
  }
  g_debug += "],\"fired\":[";
  for (size_t i = 0; i < e.fired().size(); ++i) g_debug += (i ? "," : "") + std::to_string(e.fired()[i]);
  g_debug += "],\"sounds\":[";
  for (size_t i = 0; i < e.sounds().size(); ++i) g_debug += (i ? "," : "") + std::to_string(e.sounds()[i]);
  g_debug += "],\"text\":";
  append_json_string(g_debug, e.text());
  g_debug += "}";
  return g_debug.c_str();
}

}  // extern "C"

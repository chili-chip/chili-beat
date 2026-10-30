// Chili Beat player for 32blit and the chilichip console.
//
// A handler app: ChiliChip OS lists .beat files on the CHILICHIP drive and opens
// them with this app. Started with no file (for example on desktop), it plays
// the bundled Bee Home.
#include "32blit.hpp"
#include "assets.hpp"

#include <chili_beat/engine.hpp>

#include <optional>
#include <string>

using namespace blit;

namespace {

std::optional<chili_beat::Engine> g_engine;
std::string g_error;
uint32_t g_last_ms = 0;
Pen g_pens[4];
chili_beat::Frame g_frame;
bool g_was_on[chili_beat::kChannels] = {};

class BlitHost : public chili_beat::Host {
 public:
  uint32_t delta_ms = 0;

  uint32_t delta_time_ms() override { return delta_ms; }

  uint8_t buttons() override {
    uint8_t b = 0;
    if (blit::buttons & Button::DPAD_UP) b |= chili_beat::kUp;
    if (blit::buttons & Button::DPAD_DOWN) b |= chili_beat::kDown;
    if (blit::buttons & Button::DPAD_LEFT) b |= chili_beat::kLeft;
    if (blit::buttons & Button::DPAD_RIGHT) b |= chili_beat::kRight;
    if (blit::buttons & Button::A) b |= chili_beat::kA;
    if (blit::buttons & Button::B) b |= chili_beat::kB;
    return b;
  }

  void present(const chili_beat::Frame& frame) override { g_frame = frame; }
};

BlitHost g_host;

std::string read_file(const char* path) {
  File file(path);
  if (!file.is_open()) return {};
  std::string text(file.get_length(), '\0');
  file.read(0, file.get_length(), text.data());
  return text;
}

void start(const std::string& text) {
  chili_beat::Game game;
  if (!chili_beat::load(text, game, g_error)) return;
  for (int i = 0; i < 4; ++i) {
    uint32_t c = chili_beat::kSweetie16[game.palette[i]];
    g_pens[i] = Pen(int(c >> 16), int((c >> 8) & 0xFF), int(c & 0xFF));
  }
  g_engine.emplace(std::move(game));
}

void play_channel(int index, const chili_beat::Channel& in) {
  AudioChannel& out = channels[index];
  if (in.volume == 0 || in.frequency == 0) {
    if (g_was_on[index]) out.off();
    g_was_on[index] = false;
    return;
  }
  switch (in.wave) {
    case chili_beat::Channel::Wave::Square: out.waveforms = Waveform::SQUARE; break;
    case chili_beat::Channel::Wave::Triangle: out.waveforms = Waveform::TRIANGLE; break;
    case chili_beat::Channel::Wave::Noise: out.waveforms = Waveform::NOISE; break;
  }
  out.frequency = in.frequency;
  out.volume = static_cast<uint16_t>(in.volume * 0x7f);  // keep some headroom for 4 channels
  out.attack_ms = 1;
  out.decay_ms = 1;
  out.sustain = 0xffff;
  out.release_ms = 1;
  if (!g_was_on[index] || out.adsr_phase == ADSRPhase::OFF) out.trigger_sustain();
  g_was_on[index] = true;
}

}  // namespace

void init() {
  set_screen_mode(ScreenMode::hires);
  const char* launch = get_launch_path();
  if (launch && launch[0]) {
    std::string text = read_file(launch);
    if (text.empty()) {
      g_error = std::string("Can't read ") + launch;
      return;
    }
    start(text);
  } else {
    start(std::string(reinterpret_cast<const char*>(asset_bee_home), asset_bee_home_length));
  }
}

void update(uint32_t time) {
  g_host.delta_ms = g_last_ms ? time - g_last_ms : 10;
  g_last_ms = time;
  if (!g_engine) return;
  g_engine->update(g_host);
  for (int i = 0; i < chili_beat::kChannels; ++i) play_channel(i, g_frame.channels[i]);
}

void render(uint32_t time) {
  screen.pen = Pen(0, 0, 0);
  screen.clear();
  if (!g_engine) {
    screen.pen = Pen(255, 255, 255);
    screen.text("This game can't be played:", minimal_font, Point(4, 4));
    screen.text(g_error, minimal_font, Rect(4, 16, screen.bounds.w - 8, screen.bounds.h - 20));
    return;
  }
  // Draw the 128 x 128 picture as large as fits, in the middle.
  Surface src(g_frame.pixels.data(), PixelFormat::P, Size(chili_beat::kScreenPixels, chili_beat::kScreenPixels));
  src.palette = g_pens;
  int size = std::min(screen.bounds.w, screen.bounds.h);
  screen.pen = g_pens[0];
  screen.rectangle(Rect((screen.bounds.w - size) / 2, (screen.bounds.h - size) / 2, size, size));
  screen.stretch_blit(&src, Rect(0, 0, chili_beat::kScreenPixels, chili_beat::kScreenPixels),
                      Rect((screen.bounds.w - size) / 2, (screen.bounds.h - size) / 2, size, size));
}

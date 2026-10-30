// beat-shot: plays a game with an input script and saves the screen as a PNG.
//
//   beat-shot GAME.beat OUT.png [SCRIPT] [--scale N]
//
// SCRIPT is a list of words: up down left right a b (press and release),
// wait (one step), beat (until the next world beat), and N before a word to
// repeat it. Example: "3 right up 4 beat".
#include <chili_beat/engine.hpp>

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace chili_beat;

namespace {

uint32_t crc32(const uint8_t* data, size_t n, uint32_t crc = 0) {
  crc = ~crc;
  for (size_t i = 0; i < n; ++i) {
    crc ^= data[i];
    for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
  }
  return ~crc;
}

void put32(std::vector<uint8_t>& out, uint32_t v) {
  for (int s = 24; s >= 0; s -= 8) out.push_back(static_cast<uint8_t>(v >> s));
}

void chunk(std::vector<uint8_t>& png, const char* type, const std::vector<uint8_t>& data) {
  put32(png, static_cast<uint32_t>(data.size()));
  std::vector<uint8_t> body(type, type + 4);
  body.insert(body.end(), data.begin(), data.end());
  png.insert(png.end(), body.begin(), body.end());
  put32(png, crc32(body.data(), body.size()));
}

// An RGB PNG with an uncompressed (stored) deflate stream.
std::vector<uint8_t> png(const std::vector<uint8_t>& rgb, int w, int h) {
  std::vector<uint8_t> raw;
  for (int y = 0; y < h; ++y) {
    raw.push_back(0);
    raw.insert(raw.end(), rgb.begin() + y * w * 3, rgb.begin() + (y + 1) * w * 3);
  }
  std::vector<uint8_t> z = {0x78, 0x01};
  for (size_t at = 0; at < raw.size(); at += 65535) {
    uint16_t n = static_cast<uint16_t>(std::min<size_t>(65535, raw.size() - at));
    z.push_back(at + n == raw.size());
    z.push_back(n & 0xFF);
    z.push_back(n >> 8);
    z.push_back(~n & 0xFF);
    z.push_back((~n >> 8) & 0xFF);
    z.insert(z.end(), raw.begin() + at, raw.begin() + at + n);
  }
  uint32_t a = 1, b = 0;
  for (uint8_t c : raw) {
    a = (a + c) % 65521;
    b = (b + a) % 65521;
  }
  put32(z, (b << 16) | a);

  std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  std::vector<uint8_t> header;
  put32(header, w);
  put32(header, h);
  header.insert(header.end(), {8, 2, 0, 0, 0});
  chunk(out, "IHDR", header);
  chunk(out, "IDAT", z);
  chunk(out, "IEND", {});
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  int scale = 1;
  for (size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == "--scale") {
      scale = std::max(1, std::min(8, std::stoi(args[i + 1])));
      args.erase(args.begin() + i, args.begin() + i + 2);
      break;
    }
  }
  if (args.size() < 2 || args.size() > 3) {
    std::cerr << "usage: beat-shot GAME.beat OUT.png [SCRIPT] [--scale N]\n";
    return 2;
  }
  std::ifstream in(args[0], std::ios::binary);
  std::stringstream text;
  text << in.rdbuf();
  Game game;
  std::string error;
  if (!load(text.str(), game, error)) {
    std::cerr << args[0] << ": " << error << "\n";
    return 1;
  }
  std::array<uint32_t, 4> colours;
  for (int i = 0; i < 4; ++i) colours[i] = kSweetie16[game.palette[i]];
  Engine engine(std::move(game));

  std::istringstream words(args.size() == 3 ? args[2] : "");
  std::string word;
  int repeat = 1;
  while (words >> word) {
    if (word[0] >= '0' && word[0] <= '9') {
      repeat = std::stoi(word);
      continue;
    }
    for (int i = 0; i < repeat; ++i) {
      if (word == "wait") {
        engine.step(0);
      } else if (word == "beat") {
        uint32_t start = engine.beat();
        for (int n = 0; n < 600 && engine.beat() == start; ++n) engine.step(0);
      } else {
        uint8_t b = word == "up" ? kUp : word == "down" ? kDown : word == "left" ? kLeft
                  : word == "right" ? kRight : word == "a" ? kA : word == "b" ? kB : 0;
        if (!b) {
          std::cerr << "unknown word in script: " << word << "\n";
          return 2;
        }
        engine.step(b);
        engine.step(0);
      }
    }
    repeat = 1;
  }
  // Let slides finish.
  for (int i = 0; i < 8; ++i) engine.step(0);

  Frame frame;
  engine.draw(frame);
  int size = kScreenPixels * scale;
  std::vector<uint8_t> rgb(size * size * 3);
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      uint32_t c = colours[frame.pixels[(y / scale) * kScreenPixels + x / scale]];
      uint8_t* p = &rgb[(y * size + x) * 3];
      p[0] = c >> 16;
      p[1] = (c >> 8) & 0xFF;
      p[2] = c & 0xFF;
    }
  }
  std::vector<uint8_t> out = png(rgb, size, size);
  std::ofstream file(args[1], std::ios::binary);
  file.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
  return file ? 0 : 1;
}

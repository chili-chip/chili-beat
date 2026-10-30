#include "json.hpp"

namespace chili_beat::json {

const Value* Value::get(std::string_view key) const {
  for (const auto& [k, v] : members) {
    if (k == key) return &v;
  }
  return nullptr;
}

namespace {

constexpr int kMaxDepth = 32;

struct Parser {
  std::string_view text;
  size_t pos = 0;
  std::string error;

  bool fail(const std::string& message) {
    if (error.empty()) {
      int line = 1;
      for (size_t i = 0; i < pos && i < text.size(); ++i) line += text[i] == '\n';
      error = "line " + std::to_string(line) + ": " + message;
    }
    return false;
  }

  void skip_space() {
    while (pos < text.size() &&
           (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n' || text[pos] == '\r')) {
      ++pos;
    }
  }

  bool literal(std::string_view word) {
    if (text.substr(pos, word.size()) != word) return fail("unexpected text");
    pos += word.size();
    return true;
  }

  static void append_utf8(std::string& s, uint32_t c) {
    if (c < 0x80) {
      s += static_cast<char>(c);
    } else if (c < 0x800) {
      s += static_cast<char>(0xC0 | (c >> 6));
      s += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
      s += static_cast<char>(0xE0 | (c >> 12));
      s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
      s += static_cast<char>(0x80 | (c & 0x3F));
    } else {
      s += static_cast<char>(0xF0 | (c >> 18));
      s += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
      s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
      s += static_cast<char>(0x80 | (c & 0x3F));
    }
  }

  bool hex4(uint32_t& out) {
    if (pos + 4 > text.size()) return fail("bad \\u escape");
    out = 0;
    for (int i = 0; i < 4; ++i) {
      char c = text[pos++];
      out <<= 4;
      if (c >= '0' && c <= '9') out |= c - '0';
      else if (c >= 'a' && c <= 'f') out |= c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') out |= c - 'A' + 10;
      else return fail("bad \\u escape");
    }
    return true;
  }

  bool string(std::string& out) {
    ++pos;  // opening quote
    while (pos < text.size()) {
      char c = text[pos++];
      if (c == '"') return true;
      if (static_cast<unsigned char>(c) < 0x20) return fail("control character in text");
      if (c != '\\') {
        out += c;
        continue;
      }
      if (pos >= text.size()) break;
      switch (char e = text[pos++]) {
        case '"': case '\\': case '/': out += e; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          uint32_t c1;
          if (!hex4(c1)) return false;
          if (c1 >= 0xD800 && c1 < 0xDC00 && text.substr(pos, 2) == "\\u") {
            pos += 2;
            uint32_t c2;
            if (!hex4(c2)) return false;
            c1 = 0x10000 + ((c1 - 0xD800) << 10) + (c2 - 0xDC00);
          }
          append_utf8(out, c1);
          break;
        }
        default: return fail("bad escape in text");
      }
    }
    return fail("text has no closing quote");
  }

  bool number(Value& out) {
    bool negative = text[pos] == '-';
    if (negative) ++pos;
    if (pos >= text.size() || text[pos] < '0' || text[pos] > '9') return fail("bad number");
    int64_t n = 0;
    while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
      if (n > 1'000'000'000'000) return fail("number too big");
      n = n * 10 + (text[pos++] - '0');
    }
    if (pos < text.size() && (text[pos] == '.' || text[pos] == 'e' || text[pos] == 'E')) {
      return fail("numbers must be whole numbers");
    }
    out.type = Value::Type::Int;
    out.integer = negative ? -n : n;
    return true;
  }

  bool value(Value& out, int depth) {
    if (depth > kMaxDepth) return fail("too deeply nested");
    skip_space();
    if (pos >= text.size()) return fail("unexpected end of file");
    char c = text[pos];
    if (c == '{') {
      out.type = Value::Type::Object;
      ++pos;
      skip_space();
      if (pos < text.size() && text[pos] == '}') { ++pos; return true; }
      while (true) {
        skip_space();
        if (pos >= text.size() || text[pos] != '"') return fail("expected a field name");
        std::string key;
        if (!string(key)) return false;
        if (out.get(key)) return fail("field \"" + key + "\" appears twice");
        skip_space();
        if (pos >= text.size() || text[pos] != ':') return fail("expected ':'");
        ++pos;
        Value v;
        if (!value(v, depth + 1)) return false;
        out.members.emplace_back(std::move(key), std::move(v));
        skip_space();
        if (pos < text.size() && text[pos] == ',') { ++pos; continue; }
        if (pos < text.size() && text[pos] == '}') { ++pos; return true; }
        return fail("expected ',' or '}'");
      }
    }
    if (c == '[') {
      out.type = Value::Type::Array;
      ++pos;
      skip_space();
      if (pos < text.size() && text[pos] == ']') { ++pos; return true; }
      while (true) {
        Value v;
        if (!value(v, depth + 1)) return false;
        out.items.push_back(std::move(v));
        skip_space();
        if (pos < text.size() && text[pos] == ',') { ++pos; continue; }
        if (pos < text.size() && text[pos] == ']') { ++pos; return true; }
        return fail("expected ',' or ']'");
      }
    }
    if (c == '"') {
      out.type = Value::Type::String;
      return string(out.string);
    }
    if (c == 't') { out.type = Value::Type::Bool; out.boolean = true; return literal("true"); }
    if (c == 'f') { out.type = Value::Type::Bool; return literal("false"); }
    if (c == 'n') { out.type = Value::Type::Null; return literal("null"); }
    if (c == '-' || (c >= '0' && c <= '9')) return number(out);
    return fail("unexpected character");
  }
};

}  // namespace

std::string parse(std::string_view text, Value& out) {
  Parser p{text, 0, {}};
  out = Value{};
  if (p.value(out, 0)) {
    p.skip_space();
    if (p.pos != text.size()) p.fail("extra text after the end");
  }
  return p.error;
}

}  // namespace chili_beat::json

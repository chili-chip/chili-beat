// Small JSON reader for .beat files. Numbers must be whole numbers.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace chili_beat::json {

struct Value {
  enum class Type : uint8_t { Null, Bool, Int, String, Array, Object };

  Type type = Type::Null;
  bool boolean = false;
  int64_t integer = 0;
  std::string string;
  std::vector<Value> items;                            // Array
  std::vector<std::pair<std::string, Value>> members;  // Object, in file order

  const Value* get(std::string_view key) const;
};

// Parses text into out. Returns an empty string, or an error with a line number.
std::string parse(std::string_view text, Value& out);

}  // namespace chili_beat::json

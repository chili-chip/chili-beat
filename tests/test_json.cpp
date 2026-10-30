#include "check.hpp"
#include "json.hpp"

using chili_beat::json::parse;
using chili_beat::json::Value;

TEST(json_reads_values) {
  Value v;
  CHECK_EQ(parse(R"({"a": [1, -2, true, false, null], "b": "x\né😀"})", v), "");
  CHECK(v.type == Value::Type::Object);
  const Value* a = v.get("a");
  CHECK(a && a->items.size() == 5);
  CHECK_EQ(a->items[1].integer, -2);
  CHECK(a->items[2].boolean);
  CHECK(a->items[4].type == Value::Type::Null);
  CHECK_EQ(v.get("b")->string, "x\n\xC3\xA9\xF0\x9F\x98\x80");
}

TEST(json_rejects_bad_text) {
  Value v;
  CHECK(parse("1.5", v).find("whole numbers") != std::string::npos);
  CHECK(parse(R"({"a": 1, "a": 2})", v).find("twice") != std::string::npos);
  CHECK(parse("[1, 2", v) != "");
  CHECK(parse("[1] x", v).find("extra text") != std::string::npos);
  CHECK(parse("\"abc", v) != "");
  CHECK(parse("{\n\"a\": nope}", v).find("line 2") == 0);
  std::string deep(100, '[');
  CHECK(parse(deep, v).find("nested") != std::string::npos);
}

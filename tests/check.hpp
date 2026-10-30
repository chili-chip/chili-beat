// A tiny test runner: TEST(name) { CHECK(a == b); }
#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace check {

struct Test {
  const char* name;
  std::function<void()> run;
};

inline std::vector<Test>& tests() {
  static std::vector<Test> all;
  return all;
}

inline int& failures() {
  static int n = 0;
  return n;
}

struct Register {
  Register(const char* name, std::function<void()> run) { tests().push_back({name, std::move(run)}); }
};

}  // namespace check

#define CHECK_CAT2(a, b) a##b
#define CHECK_CAT(a, b) CHECK_CAT2(a, b)
#define TEST(name)                                                       \
  static void name();                                                    \
  static check::Register CHECK_CAT(register_, name)(#name, name);        \
  static void name()

#define CHECK(expr)                                                                       \
  do {                                                                                    \
    if (!(expr)) {                                                                        \
      ++check::failures();                                                                \
      std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK(" #expr ") failed\n";          \
    }                                                                                     \
  } while (0)

#define CHECK_EQ(a, b)                                                                    \
  do {                                                                                    \
    auto check_a = (a);                                                                   \
    auto check_b = (b);                                                                   \
    if (!(check_a == check_b)) {                                                          \
      ++check::failures();                                                                \
      std::cerr << __FILE__ << ":" << __LINE__ << ": " #a " == " #b " failed: " << check_a \
                << " != " << check_b << "\n";                                             \
    }                                                                                     \
  } while (0)

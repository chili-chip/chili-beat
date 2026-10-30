#include <cstring>

#include "check.hpp"

// Runs all tests, or only those whose name contains argv[1].
int main(int argc, char** argv) {
  int run = 0;
  for (const auto& test : check::tests()) {
    if (argc > 1 && !std::strstr(test.name, argv[1])) continue;
    int before = check::failures();
    test.run();
    ++run;
    if (check::failures() != before) std::cerr << "  in " << test.name << "\n";
  }
  std::cout << run << " tests, " << check::failures() << " failed checks\n";
  return check::failures() ? 1 : 0;
}

cmake_minimum_required(VERSION 3.20)
project(chili_beat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHILI_BEAT_BUILD_TESTS "Build tests and tools" ${PROJECT_IS_TOP_LEVEL})

# The core has no platform, media or third-party dependencies.
add_library(chili_beat STATIC
  core/src/json.cpp
  core/src/load.cpp
  core/src/engine.cpp
  core/src/font.cpp
)
target_include_directories(chili_beat PUBLIC core/include PRIVATE core/src)
target_compile_options(chili_beat PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(CHILI_BEAT_BUILD_TESTS)
  enable_testing()
  add_executable(chili_beat_tests
    tests/main.cpp
    tests/test_json.cpp
    tests/test_load.cpp
    tests/test_engine.cpp
    tests/test_rules.cpp
  )
  target_link_libraries(chili_beat_tests PRIVATE chili_beat)
  target_include_directories(chili_beat_tests PRIVATE core/src)
  target_compile_definitions(chili_beat_tests PRIVATE CHILI_BEAT_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
  add_test(NAME chili_beat_tests COMMAND chili_beat_tests)

  add_executable(beat-shot tools/beat_shot.cpp)
  target_link_libraries(beat-shot PRIVATE chili_beat)
endif()

# Web player. Build with Emscripten:
#   emcmake cmake -S hosts/wasm -B build-wasm && cmake --build build-wasm
cmake_minimum_required(VERSION 3.20)
project(chili_beat_wasm LANGUAGES CXX)

add_subdirectory(../.. ${CMAKE_BINARY_DIR}/core)

add_executable(chili-beat main.cpp)
target_link_libraries(chili-beat PRIVATE chili_beat)
target_compile_options(chili-beat PRIVATE -O2)
target_link_options(chili-beat PRIVATE
  -O2
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createChiliBeat
  -sENVIRONMENT=web,worker,node
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_FUNCTIONS=_cb_load,_cb_error,_cb_step,_cb_restart,_cb_rgba,_cb_sound,_cb_debug,_malloc,_free
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,HEAPU8
)
set_target_properties(chili-beat PROPERTIES SUFFIX ".js")

# Ship the player wrapper and demo page next to the build.
configure_file(player.js ${CMAKE_BINARY_DIR}/player.js COPYONLY)
configure_file(index.html ${CMAKE_BINARY_DIR}/index.html COPYONLY)
configure_file(../../tests/data/bee-home.beat ${CMAKE_BINARY_DIR}/bee-home.beat COPYONLY)

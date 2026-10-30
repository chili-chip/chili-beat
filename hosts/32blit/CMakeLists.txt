cmake_minimum_required(VERSION 3.20)
project(chili-beat-32blit LANGUAGES C CXX ASM)

find_package(32BLIT CONFIG REQUIRED PATHS ../../../32blit-sdk $ENV{PATH_32BLIT_SDK})

# The engine core, from this repo.
add_subdirectory(../.. ${CMAKE_BINARY_DIR}/core)

blit_executable(chili-beat game.cpp)
set_property(TARGET chili-beat PROPERTY CXX_STANDARD 20)
target_link_libraries(chili-beat chili_beat)
blit_assets_yaml(chili-beat assets.yml)
blit_metadata(chili-beat metadata.yml)

if(32BLIT_PICO)
  target_compile_options(chili_beat PRIVATE $<$<NOT:$<CONFIG:Debug>>:-O2>)
endif()

install(FILES ../../LICENSE ../../README.md DESTINATION .)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)

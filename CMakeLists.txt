cmake_minimum_required(VERSION 3.16)
project(NatureRealityEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Engine library
# ---------------------------------------------------------------------------

add_library(nre_engine STATIC
    engine/nature/LSystem.cpp
    engine/nature/TreeRenderer.cpp
)

target_include_directories(nre_engine PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_compile_options(nre_engine PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# ---------------------------------------------------------------------------
# Artemis demo executable
# ---------------------------------------------------------------------------

add_executable(artemis_demo
    examples/artemis_demo/main.cpp
)

target_link_libraries(artemis_demo PRIVATE nre_engine)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

add_executable(test_lsystem tests/test_lsystem.cpp)
target_link_libraries(test_lsystem PRIVATE nre_engine)
add_test(NAME LSystemTests COMMAND test_lsystem)

add_executable(test_treerenderer tests/test_treerenderer.cpp)
target_link_libraries(test_treerenderer PRIVATE nre_engine)
add_test(NAME TreeRendererTests COMMAND test_treerenderer)

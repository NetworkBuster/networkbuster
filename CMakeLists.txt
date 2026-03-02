cmake_minimum_required(VERSION 3.14)
project(NatureRealityEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Neural network library
add_library(nre_neural
    engine/neural/NeuralNetwork.cpp
)
target_include_directories(nre_neural PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Neural network demo
add_executable(neural_demo examples/neural_demo/main.cpp)
target_link_libraries(neural_demo PRIVATE nre_neural)

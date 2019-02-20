cmake_minimum_required(VERSION 3.13)
project(AnalisadorOpenGL)

set(CMAKE_CXX_STANDARD 11)

add_executable(AnalisadorOpenGL main.cpp shader.h camera.h model_loading.h model_loading.cpp glad.h glad.c lista.cpp lista.h structs.h structs.cpp)

target_link_libraries(AnalisadorOpenGL glfw3)
target_link_libraries(AnalisadorOpenGL assimp)
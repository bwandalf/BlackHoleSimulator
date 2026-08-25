cmake_minimum_required(VERSION 4.0)
project(BlackHoleSim C)

set(CMAKE_C_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(BlackHoleSim main.c)
target_link_libraries(BlackHoleSim OpenGL::GL GLEW::GLEW glfw m)

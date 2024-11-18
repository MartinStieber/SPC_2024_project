cmake_minimum_required(VERSION 3.29)
project(SPC_2024_project C)

set(CMAKE_C_STANDARD 23)

add_executable(SPC_2024_project main.c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
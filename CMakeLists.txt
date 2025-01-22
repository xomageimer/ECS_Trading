cmake_minimum_required(VERSION 3.17)
project(ECS_Trading_EnTT)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/entt-master/src)

add_executable(ECS_Trading_EnTT main.cpp components.hpp systems.hpp)

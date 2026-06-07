cmake_minimum_required(VERSION 3.25)

project(minicc)

add_executable(minicc)

target_compile_features(minicc PRIVATE cxx_std_23)

target_include_directories(minicc PRIVATE include)

target_sources(minicc PRIVATE main.cpp)
cmake_minimum_required(VERSION 3.15)
project(hw4)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(sim_main
    core_api.cpp
    core_api.h
    main.c
    sim_api.c
    sim_api.h)

cmake_minimum_required(VERSION 3.15)
project(SO-Project C)

set(CMAKE_C_STANDARD 11)

add_executable(Airport
        logging.c ControlTower.c ControlTower.h SimulationManager.c SimulationManager.h
        structs.h SimulationUtils.c SimulationUtils.h)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
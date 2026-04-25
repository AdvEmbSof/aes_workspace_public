# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(string_test)

FILE(GLOB app_sources src/*.cpp ../../src/car_system.cpp ../../src/task_recorder.cpp ../../src/sporadic_task_generator.cpp ../../src/deferrable_server.cpp ../../src/userspace/init_domain.cpp)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ../../src/ )

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(
    multi_tasking
    VERSION 0.1
    DESCRIPTION "CPP multi_tasking application."
    LANGUAGES C CXX
)

target_sources(
    app
    PRIVATE
    src/main.cpp
    src/clock.cpp
    src/deadlock.cpp
    src/wait_on_button.cpp
    src/work_on_button.cpp
)
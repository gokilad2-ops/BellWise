cmake_minimum_required(VERSION 3.22.1)
project("schedule_core")

add_library(
        schedule_core
        SHARED
        schedule_core.cpp
        jni_bridge.cpp
)

find_library(log-lib log)

target_link_libraries(
        schedule_core
        ${log-lib}
)

target_compile_features(schedule_core PRIVATE cxx_std_17)

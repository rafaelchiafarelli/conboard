cmake_minimum_required(VERSION 3.13)
project(devprobe CXX)

# Compiles the shared classifier source directly (no dependency on libcommon.so
# at runtime), so devprobe stays a self-contained, copy-anywhere diagnostic.
set(COMMON_DIR ${CMAKE_SOURCE_DIR}/../../LowLevel/Common)

add_executable(devprobe
    src/devprobe.cpp
    ${COMMON_DIR}/src/deviceDetect.cpp)

target_include_directories(devprobe PRIVATE ${COMMON_DIR}/include)
set_target_properties(devprobe PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

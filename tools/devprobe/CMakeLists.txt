cmake_minimum_required(VERSION 3.13)
project(devprobe CXX)

# Standalone diagnostic tool. No external dependencies (kernel headers only),
# so it also compiles directly with:
#     g++ -std=c++17 -O2 src/devprobe.cpp -o devprobe
add_executable(devprobe src/devprobe.cpp)
set_target_properties(devprobe PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

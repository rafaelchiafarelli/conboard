project(conJoyS)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../Common/include)


file(GLOB SOURCES "src/*.cpp")


add_executable(conJoyS ${SOURCES})

target_link_libraries(conJoyS ${CMAKE_SOURCE_DIR}/../Common/build/libcommon.so)
# Find libcommon.so relative to this binary at runtime (see launcher CMakeLists).
set_target_properties(conJoyS PROPERTIES BUILD_RPATH "$ORIGIN/../../Common/build")
target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include")
target_link_libraries(conJoyS stdc++fs)
# evdev input is plain read()/poll() -- no ALSA needed (unlike conMIDI).
target_link_libraries(conJoyS pthread)
set_target_properties(conJoyS PROPERTIES
                    CXX_STANDARD 17
                    CXX_STANDARD_REQUIRED ON
                    )

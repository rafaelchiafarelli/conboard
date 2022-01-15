project(TouchScreen)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../Common/include)

file(GLOB SOURCES "src/*.cpp")

add_executable(TouchScreen ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include")

find_package(cppzmq)
target_link_libraries(TouchScreen cppzmq)
target_link_libraries(TouchScreen ${CMAKE_SOURCE_DIR}/../Common/build/libcommon.so)
target_link_libraries(TouchScreen stdc++fs)
target_link_libraries(TouchScreen asound)
target_link_libraries(TouchScreen pthread)
set_target_properties(TouchScreen PROPERTIES
                                CXX_STANDARD 17
                                CXX_STANDARD_REQUIRED ON
                                )


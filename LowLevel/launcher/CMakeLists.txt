project(launcher)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../Common/include)

file(GLOB SOURCES "src/*.cpp")

add_executable(launcher ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include")

target_link_libraries(launcher ${CMAKE_SOURCE_DIR}/../Common/build/libcommon.so)
target_link_libraries(launcher stdc++fs)
target_link_libraries(launcher asound)
target_link_libraries(launcher pthread)
set_target_properties(launcher PROPERTIES
                                CXX_STANDARD 17
                                CXX_STANDARD_REQUIRED ON
                                )


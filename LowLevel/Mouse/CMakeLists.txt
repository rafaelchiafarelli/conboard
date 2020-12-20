project(conMouse)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../Common/include)

file(GLOB SOURCES "src/*.cpp")

add_executable(conMouse ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include")

target_link_libraries(conMouse ${CMAKE_SOURCE_DIR}/../Common/build/libcommon.so)
target_link_libraries(conMouse stdc++fs)
target_link_libraries(conMouse asound)
target_link_libraries(conMouse pthread)
set_target_properties(conMouse PROPERTIES
                                CXX_STANDARD 17
                                CXX_STANDARD_REQUIRED ON
                                )


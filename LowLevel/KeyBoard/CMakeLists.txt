project(conKeyB)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)


file(GLOB SOURCES "src/*.cpp")


add_executable(conKeyB ${SOURCES})

target_link_libraries(conKeyB ${CMAKE_SOURCE_DIR}/../Common/build/libcommon.so)
target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include")
target_link_libraries(conKeyB stdc++fs)
target_link_libraries(conKeyB asound)
target_link_libraries(conKeyB pthread)
set_target_properties(conKeyB PROPERTIES
                    CXX_STANDARD 17
                    CXX_STANDARD_REQUIRED ON
                    )

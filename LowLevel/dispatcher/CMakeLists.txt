project(dispatcher)
cmake_minimum_required(VERSION 3.13)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../Common/include)


file(GLOB SOURCES "src/*.cpp")

add_executable(dispatcher ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "/usr/include")

find_package(cppzmq)

target_link_libraries(dispatcher cppzmq)
target_link_libraries(dispatcher ${CMAKE_SOURCE_DIR}/../Common/build/libcommon.so)
# Find libcommon.so relative to this binary at runtime (see launcher CMakeLists).
set_target_properties(dispatcher PROPERTIES BUILD_RPATH "$ORIGIN/../../Common/build")
target_link_libraries(dispatcher uuid)

target_link_libraries(dispatcher boost_system)
target_link_libraries(dispatcher stdc++fs)
target_link_libraries(dispatcher pthread)
set_target_properties(dispatcher PROPERTIES
                                CXX_STANDARD 17
                                CXX_STANDARD_REQUIRED ON
                                )


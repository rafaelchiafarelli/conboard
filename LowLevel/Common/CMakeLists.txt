project(common)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)
find_package(cppzmq)

file(GLOB SOURCES "src/*.cpp")

add_library(common SHARED ${SOURCES} )
target_link_libraries(common cppzmq)

set_target_properties(common PROPERTIES
            CXX_STANDARD 17
	        CXX_STANDARD_REQUIRED ON
            )

project(common)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)


file(GLOB SOURCES "src/*.cpp")

add_library(common SHARED ${SOURCES} )


set_target_properties(common PROPERTIES
            CXX_STANDARD 17
	        CXX_STANDARD_REQUIRED ON
            )

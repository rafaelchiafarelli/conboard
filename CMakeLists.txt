project(conboard)
cmake_minimum_required(VERSION 3.13)
include_directories(${CMAKE_SOURCE_DIR}/include)


file(GLOB SOURCES "src/*.cpp")

add_executable(conboard ${SOURCES})
target_link_libraries(conboard stdc++fs)

set_target_properties(conboard PROPERTIES
            CXX_STANDARD 17
	    CXX_STANDARD_REQUIRED ON
            )

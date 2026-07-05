# conboard backend (C++) — SKELETON (2026-06-30)
# Not yet wired into build-cross.sh. Real sources/structs come from ../harpia.
cmake_minimum_required(VERSION 3.16)
project(conboard_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies (planned) -------------------------------------------------
# Crow (header-only HTTP) is vendored as a submodule under LowLevel/Common/Crow.
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../LowLevel/Common/Crow ...)
#
# libpqxx (PostgreSQL C++ client) — find_package(libpqxx) or pkg-config.
# find_package(libpqxx REQUIRED)
#
# Shared types live in LowLevel/Common (DeviceEngine, jsonParser, etc.).
# harpia-generated message structs + JSON + DB schema land here once codegen is ready.

add_executable(conboard_backend
    src/main.cpp
)

target_include_directories(conboard_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../LowLevel/Common/Crow/include
)

# target_link_libraries(conboard_backend PRIVATE libpqxx::pqxx common)

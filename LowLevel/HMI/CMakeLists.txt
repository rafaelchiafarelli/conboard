# conHMI -- the screen + buttons + encoders local UI process.
#
# Deliberately independent of the rest of LowLevel/: no libcommon.so link, no
# ../Common/include path. Its only deps are LVGL (vendored submodule),
# libgpiod (SPI panel GPIO lines + rotary encoders/buttons), libcurl (the
# REST client -- the module's only source of domain data), and a
# single-header JSON library (vendored, like doctest.h/zmq.hpp elsewhere in
# the repo). See the plan doc for why.
project(conHMI)
cmake_minimum_required(VERSION 3.13)

include_directories(${CMAKE_SOURCE_DIR}/include)

# LVGL needs its lv_conf.h found before any lvgl header is processed; this
# cache var (read by lvgl/env_support/cmake/os_desktop.cmake) is how the
# upstream build asks for a project-supplied one.
set(LV_BUILD_CONF_PATH ${CMAKE_SOURCE_DIR}/lv_conf.h CACHE PATH "" FORCE)
# The demo/example sources aren't used by conHMI; skip compiling them (the
# option names LVGL's own os_desktop.cmake actually reads are CONFIG_-prefixed).
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(lvgl)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)

file(GLOB SOURCES "src/*.cpp")
add_executable(conHMI ${SOURCES})

target_link_libraries(conHMI
    lvgl
    PkgConfig::GPIOD
    PkgConfig::CURL
    pthread)

set_target_properties(conHMI PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

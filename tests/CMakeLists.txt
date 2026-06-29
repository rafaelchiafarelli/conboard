cmake_minimum_required(VERSION 3.13)
project(conboard_tests CXX)

# Tests compile the source-under-test directly (no libcommon.so / zmq link), so
# the suite stays dependency-light and runs the same on host or under QEMU.
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../LowLevel/Common)

add_executable(conboard_tests
    test_devicedetect.cpp
    test_midimap.cpp
    test_jsonparser.cpp
    test_launchermatch.cpp
    test_evmatch.cpp
    test_holdgen.cpp
    ${COMMON_DIR}/src/deviceDetect.cpp
    ${COMMON_DIR}/src/midiMap.cpp
    ${COMMON_DIR}/src/jsonParser.cpp
    ${COMMON_DIR}/src/launcherMatch.cpp
    ${COMMON_DIR}/src/evMatch.cpp
    ${COMMON_DIR}/src/holdGen.cpp)

target_include_directories(conboard_tests PRIVATE
    ${COMMON_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})            # for doctest.h

set_target_properties(conboard_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

# Register one CTest entry per doctest suite, so `ctest -R joystick` works too.
enable_testing()
foreach(suite classifier joystick usb midi json launch filter evmatch holdgen)
    add_test(NAME ${suite}
             COMMAND conboard_tests --test-suite=${suite} --no-intro)
endforeach()

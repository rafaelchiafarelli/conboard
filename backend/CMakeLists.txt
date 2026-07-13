# conboard backend (C++). Management API + dispatcher relay.
#
# Consumes the harpia-generated project under backend/generated/ (the examples/
# consumer pattern from ../harpia/USAGE.md section 6): include generated/cpp, compile
# the protobuf .pb.cc, vendor crow/asio/tinyxml2 from the generated tree, and link
# SOCI (sqlite backend), protobuf and gRPC from system packages.
#
# NB: the DB client is SOCI, not libpqxx (harpia emits soci::session& everywhere).
# conboard targets the sqlite dialect (device-local embedded DB).
cmake_minimum_required(VERSION 3.16)
project(conboard_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GEN    "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set(GENCPP "${GEN}/generated/cpp")
if(NOT EXISTS "${GENCPP}")
    message(FATAL_ERROR "backend/generated/ missing -- run backend/generate.sh first.")
endif()

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPCPP REQUIRED grpc++)
# libudev: the device-inventory endpoint (src/devices.cpp) enumerates attached
# devices and reads their udev ID_* properties (the same tags boards match on).
pkg_check_modules(UDEV REQUIRED libudev)

# SOCI + the sqlite3 backend + libsqlite3 (system libs; no reliable CMake config).
find_library(SOCI_CORE    NAMES soci_core    REQUIRED)
find_library(SOCI_SQLITE3 NAMES soci_sqlite3 REQUIRED)
find_library(SQLITE3      NAMES sqlite3       REQUIRED)

# All generated protobuf/gRPC C++ (message + *_service + framework protos). Compiling
# the committed .pb.cc keeps the build self-contained; if a target's protoc differs
# from its libprotobuf, regenerate from proto/ instead (see CP7 / build-cross).
file(GLOB PB_SRCS "${GENCPP}/protofiles/*.pb.cc")

# tinyxml2 backs the REST content-negotiation path; vendored in the generated tree.
add_library(conboard_tinyxml2 STATIC "${GEN}/third_party/tinyxml2/tinyxml2.cpp")
target_include_directories(conboard_tinyxml2 PUBLIC "${GEN}/third_party/tinyxml2")

# main.cpp + one TU per entity (src/entity_*.cpp); see include/conboard_entities.h.
file(GLOB APP_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# The device classifier is the SAME code the launcher/devprobe use, compiled in
# directly so the inventory endpoint can never disagree with the runtime.
set(CONDETECT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../LowLevel/Common/src/deviceDetect.cpp")

add_executable(conboard_backend
    ${APP_SRCS}
    ${PB_SRCS}
    ${CONDETECT_SRC}
)
target_include_directories(conboard_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../LowLevel/Common/include  # rapidjson (deploy.cpp)
    ${GENCPP}                       # db/ json/ rest/ grpc/ protofiles/ ...
    ${GEN}/third_party/crow         # header-only Crow (REST + websocket)
    ${GEN}/third_party/asio         # standalone asio (Crow transport)
    ${Protobuf_INCLUDE_DIRS}
    ${GRPCPP_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)
target_link_directories(conboard_backend PRIVATE ${GRPCPP_LIBRARY_DIRS} ${UDEV_LIBRARY_DIRS})
target_link_libraries(conboard_backend PRIVATE
    conboard_tinyxml2
    ${SOCI_CORE} ${SOCI_SQLITE3} ${SQLITE3}
    protobuf::libprotobuf
    ${GRPCPP_LIBRARIES}
    ${UDEV_LIBRARIES}
    Threads::Threads ${CMAKE_DL_LIBS}
)

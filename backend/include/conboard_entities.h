// Per-entity registration seam.
//
// Each harpia entity gets its OWN translation unit (src/entity_<name>.cpp) that
// includes only that entity's generated headers. This is required: every entity's
// generated json/*.h defines an identically-signed harpia::json::is_valid_json,
// so including more than one entity's headers in a single TU is an ODR clash. Keeping
// one entity per TU (and main.cpp free of generated includes) sidesteps it.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <soci/soci.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/service_type.h>   // complete grpc::Service (unique_ptr dtor below)
#include "crow.h"

namespace conboard {

// Everything an entity needs to mount itself onto the servers main() owns.
struct Registrar {
    crow::SimpleApp& app;                                   // REST + ws host
    ::soci::session& rest_db;                               // REST/create_table session
    std::string base;                                       // REST base path
    grpc::ServerBuilder& gb;                                // gRPC builder (pre-start)
    ::soci::session& grpc_db;                               // gRPC session
    std::vector<std::unique_ptr<grpc::Service>>& grpc_services;  // keeps services alive
};

// Each creates its table (idempotent), registers its REST routes, and registers +
// retains its gRPC service. Defined in the matching src/entity_*.cpp.
void register_board(Registrar&);
void register_mode(Registrar&);
void register_rule(Registrar&);
void register_trigger(Registrar&);
void register_output_action(Registrar&);

// Axis C: deploy an authored profile to the realtime path. Not a harpia entity --
// hand-written. POST <base>/deploy with a board JSON (the boards/*.json shape);
// writes it into the on-device boards dir and triggers a handler reload. Defined in
// src/deploy.cpp.
void register_deploy(crow::SimpleApp& app, const std::string& base);

// Inverse of deploy: POST <base>/undeploy with a board JSON removes its on-device
// profile file and stops its handler service (so the hardware stops). src/deploy.cpp.
void register_undeploy(crow::SimpleApp& app, const std::string& base);

// Device inventory: GET <base>/devices lists attached USB/input devices, classifies
// each by type, and marks whether a boards/*.json profile already matches it (so the
// console can offer the undesignated ones in its add-device flow). Hand-written; uses
// libudev + the shared condetect classifier. Defined in src/devices.cpp.
void register_devices(crow::SimpleApp& app, const std::string& base);

}  // namespace conboard

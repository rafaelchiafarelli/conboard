// conboard backend -- management API (REST + gRPC) over the harpia-generated
// rules-library, plus a websocket seam for the dispatcher event relay.
//
// The message structs, JSON adapters, SOCI-backed CRUDL DAOs, REST (Crow) bindings
// and gRPC services all come from backend/generated/ (harpia, a black box). This
// file is the thin application host: it opens the SQLite session(s) and mounts the
// generated surfaces (via the per-entity TUs in entity_*.cpp) on servers it owns.
// It includes NO generated headers itself -- see include/conboard_entities.h.
//
// It is NOT in the realtime device path (that is LowLevel/). The rules library is
// device-local, single-writer, authoring-only -- so the concurrency model here is
// deliberately simple (see the note at the servers below).
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <grpcpp/grpcpp.h>
#include "crow.h"

#include "conboard_entities.h"

namespace {

std::string env_or(const char* k, const std::string& def) {
    const char* v = std::getenv(k);
    return (v && *v) ? std::string(v) : def;
}
int env_int(const char* k, int def) {
    const char* v = std::getenv(k);
    return (v && *v) ? std::atoi(v) : def;
}

// Open a SOCI/SQLite session and make cross-connection contention tolerable.
void open_session(soci::session& db, const std::string& path) {
    db.open(soci::sqlite3, path);
    db << "PRAGMA busy_timeout = 5000";   // wait, don't fail, on a held write lock
    db << "PRAGMA foreign_keys = ON";
}

}  // namespace

int main() {
    const std::string db_path   = env_or("CONBOARD_DB", "conboard.db");
    const std::string http_host = env_or("CONBOARD_HTTP_HOST", "127.0.0.1");
    const int         http_port = env_int("CONBOARD_HTTP_PORT", 8080);
    const std::string grpc_addr = env_or("CONBOARD_GRPC_ADDR", "127.0.0.1:50051");
    const std::string api_base  = env_or("CONBOARD_API_BASE", "/api/v1");

    // Concurrency model (intentionally simple; authoring workload, low frequency):
    // REST and gRPC each own a SEPARATE SQLite connection to the same file, and each
    // server is single-threaded on its session, so no session is touched by two
    // threads at once; cross-connection writes are serialised by SQLite's file lock
    // (+ busy_timeout). Move to a soci::connection_pool if real concurrency is needed.
    soci::session rest_db, grpc_db;
    try {
        open_session(rest_db, db_path);
        open_session(grpc_db, db_path);
    } catch (const std::exception& e) {
        std::cerr << "[backend] DB open failed (" << db_path << "): " << e.what() << "\n";
        return 1;
    }
    std::cerr << "[backend] SQLite: " << db_path << "\n";

    // ---- servers the app owns ----------------------------------------------
    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Warning);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(grpc_addr, grpc::InsecureServerCredentials());
    std::vector<std::unique_ptr<grpc::Service>> grpc_services;

    // ---- mount every entity (REST route + create_table + gRPC service) ------
    // Each register_* creates its table (idempotent), adds GET/POST/PUT/DELETE
    // <base>/<entity>[/<id>] gated by the generated X-User/X-Pswd headers, and
    // registers its gRPC service. conboard's own auth layers in front of REST later.
    conboard::Registrar reg{app, rest_db, api_base, builder, grpc_db, grpc_services};
    try {
        conboard::register_board(reg);
        conboard::register_mode(reg);
        conboard::register_rule(reg);
        conboard::register_trigger(reg);
        conboard::register_output_action(reg);
    } catch (const std::exception& e) {
        std::cerr << "[backend] entity registration failed: " << e.what() << "\n";
        return 1;
    }

    // Axis C: deploy an authored profile to the realtime path (write boards/*.json +
    // reload). Hand-written, not a harpia entity.
    conboard::register_deploy(app, api_base);

    // Device inventory for the console's add-device flow. Hand-written.
    conboard::register_devices(app, api_base);

    CROW_ROUTE(app, "/healthz")([]{ return "ok"; });

    // ---- Websocket seam for the dispatcher event relay ----------------------
    // The realtime device stream arrives from LowLevel/dispatcher over ZMQ (see
    // INTERFACE.md). This endpoint is where the backend will fan that stream out to
    // connected frontend clients. The ZMQ consumer thread is TODO (INTERFACE.md v0
    // framing); today the endpoint accepts + tracks clients so the relay can be
    // dropped in without touching route wiring.
    static std::mutex ws_mu;
    static std::set<crow::websocket::connection*> ws_clients;
    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([](crow::websocket::connection& c) {
            std::lock_guard<std::mutex> lk(ws_mu); ws_clients.insert(&c);
        })
        .onclose([](crow::websocket::connection& c, const std::string&, uint16_t) {
            std::lock_guard<std::mutex> lk(ws_mu); ws_clients.erase(&c);
        })
        .onmessage([](crow::websocket::connection&, const std::string&, bool) {
            // Inbound frontend->backend messages unused for now; relay is one-way.
        });

    // ---- start gRPC in the background (scaffolding for the future dispatcher-
    // over-gRPC migration; not consumed by the frontend yet) ------------------
    std::unique_ptr<grpc::Server> grpc_server(builder.BuildAndStart());
    std::thread grpc_thread;
    if (grpc_server) {
        std::cerr << "[backend] gRPC:  " << grpc_addr << "\n";
        grpc_thread = std::thread([&]{ grpc_server->Wait(); });
    } else {
        std::cerr << "[backend] WARNING: gRPC failed to bind " << grpc_addr << "\n";
    }

    // ---- serve REST in the foreground (single-threaded; its session is never
    // shared across threads) --------------------------------------------------
    std::cerr << "[backend] REST:  http://" << http_host << ":" << http_port
              << api_base << "  (ws: /ws)\n";
    app.bindaddr(http_host).port(http_port).run();

    if (grpc_server) { grpc_server->Shutdown(); }
    if (grpc_thread.joinable()) { grpc_thread.join(); }
    return 0;
}

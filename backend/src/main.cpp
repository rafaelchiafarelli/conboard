// conboard backend — SKELETON (2026-06-30)
//
// Target: a Crow HTTP/JSON server exposing the management API (CRUD over
// devices/profiles/rules/actions/modes + "copy rules A->B"), backed by a
// PostgreSQL rules library (via libpqxx), and relaying the dispatcher's ZMQ
// event stream to the frontend over a websocket.
//
// This is intentionally a placeholder. The request/response message types,
// their JSON (de)serialization, and the DB schema are to be GENERATED from
// ../harpia, not hand-written here. See backend/README.md.
//
// #include "crow.h"
//
// int main() {
//     crow::SimpleApp app;
//
//     // CROW_ROUTE(app, "/api/rules")            // list/create rules
//     // CROW_ROUTE(app, "/api/rules/<int>")      // read/update/delete a rule
//     // CROW_ROUTE(app, "/api/rules/copy")       // copy a rule set A -> B
//     // CROW_ROUTE(app, "/api/devices")          // attached devices + identity + mode
//     // CROW_ROUTE(app, "/ws")                   // websocket: dispatcher event relay
//
//     app.port(8080).multithreaded().run();
// }

int main() {
    // TODO: stand up Crow app + libpqxx connection + dispatcher (ZMQ) consumer.
    // Message types and DB schema come from harpia codegen.
    return 0;
}

// Registration TU for the "trigger" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/trigger_5a67e5f27cce34a1ec5ac267a70f5d87_rest.h"
#include "grpc/trigger_5a67e5f27cce34a1ec5ac267a70f5d87_grpc.h"

namespace conboard {

void register_trigger(Registrar& r) {
    harpia::db::trigger_dao(r.rest_db).create_table();
    harpia::rest::register_trigger(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::trigger_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

// Registration TU for the "hmi_binding" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/hmi_binding_7fb7af9d1e69abe2d7a6e81c4a2d0c2f_rest.h"
#include "grpc/hmi_binding_7fb7af9d1e69abe2d7a6e81c4a2d0c2f_grpc.h"

namespace conboard {

void register_hmi_binding(Registrar& r) {
    harpia::db::hmi_binding_dao(r.rest_db).create_table();
    harpia::rest::register_hmi_binding(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::hmi_binding_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

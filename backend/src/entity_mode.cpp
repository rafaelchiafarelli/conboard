// Registration TU for the "mode" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/mode_69421342752d53d5f274b99a6a0c123e_rest.h"
#include "grpc/mode_69421342752d53d5f274b99a6a0c123e_grpc.h"

namespace conboard {

void register_mode(Registrar& r) {
    harpia::db::mode_dao(r.rest_db).create_table();
    harpia::rest::register_mode(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::mode_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

// Registration TU for the "output_action" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/output_action_69421342752d53d5f274b99a6a0c123e_rest.h"
#include "grpc/output_action_69421342752d53d5f274b99a6a0c123e_grpc.h"

namespace conboard {

void register_output_action(Registrar& r) {
    harpia::db::output_action_dao(r.rest_db).create_table();
    harpia::rest::register_output_action(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::output_action_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

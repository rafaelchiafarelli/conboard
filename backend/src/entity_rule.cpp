// Registration TU for the "rule" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/rule_9f20d5d43738774941f9898b22cf2cf2_rest.h"
#include "grpc/rule_9f20d5d43738774941f9898b22cf2cf2_grpc.h"

namespace conboard {

void register_rule(Registrar& r) {
    harpia::db::rule_dao(r.rest_db).create_table();
    harpia::rest::register_rule(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::rule_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

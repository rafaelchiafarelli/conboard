// Registration TU for the "rule" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/rule_1bf812ac18b80d4a5ea4d51e6bfb7f58_rest.h"
#include "grpc/rule_1bf812ac18b80d4a5ea4d51e6bfb7f58_grpc.h"

namespace conboard {

void register_rule(Registrar& r) {
    harpia::db::rule_dao(r.rest_db).create_table();
    harpia::rest::register_rule(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::rule_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

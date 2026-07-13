// Registration TU for the "board" entity. Includes ONLY this entity's generated
// headers (see include/conboard_entities.h for why one entity per TU).
#include "conboard_entities.h"

#include "rest/board_1bf812ac18b80d4a5ea4d51e6bfb7f58_rest.h"
#include "grpc/board_1bf812ac18b80d4a5ea4d51e6bfb7f58_grpc.h"

namespace conboard {

void register_board(Registrar& r) {
    harpia::db::board_dao(r.rest_db).create_table();
    harpia::rest::register_board(r.app, r.rest_db, r.base);
    auto svc = std::make_unique<harpia::grpc_svc::board_service>(r.grpc_db);
    r.gb.RegisterService(svc.get());
    r.grpc_services.push_back(std::move(svc));
}

}  // namespace conboard

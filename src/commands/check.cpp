#include <iostream>

#include "command_support.hpp"

int cmd_check(const Context& ctx) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;

    try {
        NotionClient client(cfg.effective_token());
        nlohmann::json db = client.get("/databases/" + cfg.database_id);
        std::vector<std::string> problems = schema::validate(db);

        if (ctx.json) {
            nlohmann::json j = {{"ok", problems.empty()},
                                {"database_id", cfg.database_id},
                                {"problems", problems}};
            cmd::ok_json(j);
        } else if (problems.empty()) {
            std::cout << "[ntt] 스키마 정상 — task-tracker DB가 올바릅니다.\n";
        } else {
            std::cerr << "[ntt] 스키마 문제 발견:\n";
            for (const auto& p : problems) std::cerr << "  - " << p << "\n";
        }
        return problems.empty() ? 0 : 2;  // exit 2 = schema invalid
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

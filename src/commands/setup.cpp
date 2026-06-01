#include <iostream>

#include "command_support.hpp"

// Add any missing schema fields to the already-configured (existing) DB.
// Existing properties are never modified; only absent fields are added.
int cmd_setup(const Context& ctx) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;

    try {
        NotionClient client(cfg.effective_token());
        nlohmann::json db = client.get("/databases/" + cfg.database_id);

        auto [props, added] = schema::missing_properties(db);

        if (added.empty()) {
            if (ctx.json)
                cmd::ok_json({{"ok", true}, {"added", nlohmann::json::array()}, {"message", "이미 모든 필드가 있습니다."}});
            else
                std::cout << "[ntt] 추가할 필드 없음 — 스키마가 이미 완비되어 있습니다.\n";
            return 0;
        }

        client.patch("/databases/" + cfg.database_id, {{"properties", props}});

        if (ctx.json) {
            cmd::ok_json({{"ok", true}, {"added", added}});
        } else {
            std::cout << "[ntt] 누락 필드 추가 완료:\n";
            for (const auto& name : added) std::cout << "  + " << name << "\n";
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    } catch (const std::exception& e) {
        return cmd::fail(ctx, std::string("내부 오류: ") + e.what());
    }
}

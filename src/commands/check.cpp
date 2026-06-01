#include <iostream>
#include <vector>

#include "command_support.hpp"

int cmd_check(const Context& ctx) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;

    try {
        NotionClient client(cfg.effective_token());
        nlohmann::json db = client.get("/databases/" + cfg.database_id);
        schema::Resolved s = schema::resolve(db);

        // Per-field mapping (logical field -> matched column, or "" if missing).
        // 제목 is the page title (any title-type property), 마감일 included.
        std::vector<std::pair<std::string, std::string>> mapping = {
            {"제목", s.title},     {"식별용 ID", s.id}, {"수행기간", s.period},
            {"마감일", s.deadline}, {"진행 상황", s.status}, {"우선 순위", s.priority},
        };

        std::vector<std::string> missing;
        for (const auto& [field, col] : mapping)
            if (col.empty()) missing.push_back(field);

        const bool ok = missing.empty();

        if (ctx.json) {
            nlohmann::json jmap = nlohmann::json::object();
            for (const auto& [field, col] : mapping) jmap[field] = col;  // "" = 없음
            cmd::ok_json({{"ok", ok},
                          {"database_id", cfg.database_id},
                          {"mapping", jmap},
                          {"missing", missing}});
        } else {
            std::cout << "[ntt] 스키마 매핑:\n";
            for (const auto& [field, col] : mapping)
                std::cout << "  " << field << " : "
                          << (col.empty() ? "(없음)" : "→ " + col) << "\n";
            if (ok)
                std::cout << "[ntt] 정상 — 모든 필드가 매핑되었습니다.\n";
            else
                std::cerr << "[ntt] 누락: 'ntt setup'으로 추가하세요.\n";
        }
        return ok ? 0 : 2;  // exit 2 = schema invalid
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    } catch (const std::exception& e) {
        return cmd::fail(ctx, std::string("내부 오류: ") + e.what());
    }
}

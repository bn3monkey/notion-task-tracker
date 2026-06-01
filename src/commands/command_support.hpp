#pragma once
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "../config.hpp"
#include "../notion_client.hpp"
#include "../schema.hpp"
#include "commands.hpp"

// Small helpers shared by command implementations.
namespace cmd {

// Emit an error and return the given exit code. Honors --json.
inline int fail(const Context& ctx, const std::string& message, int code = 1) {
    if (ctx.json) {
        nlohmann::json j = {{"ok", false}, {"error", message}};
        std::cout << j.dump() << "\n";
    } else {
        std::cerr << "[ntt] 오류: " << message << "\n";
    }
    return code;
}

// Emit a success object (--json) or nothing/human text handled by caller.
inline void ok_json(const nlohmann::json& obj) { std::cout << obj.dump() << "\n"; }

// Ensure a Notion token is available; print error if not.
inline bool need_token(const Context& ctx, const Config& cfg) {
    if (cfg.effective_token().empty()) {
        fail(ctx, "Notion 토큰이 없습니다. 'ntt init --token <token>' 또는 NTT_NOTION_TOKEN 환경변수를 설정하세요.");
        return false;
    }
    return true;
}

// Ensure a database id is configured; print error if not.
inline bool need_database(const Context& ctx, const Config& cfg) {
    if (cfg.database_id.empty()) {
        fail(ctx, "database_id가 설정되지 않았습니다. 'ntt setup --parent <page_id>' 또는 'ntt init --db <id>'를 실행하세요.");
        return false;
    }
    return true;
}

// Find a single page in the configured DB by its 식별용 ID property.
// Returns the page JSON, or a null json if not found.
inline nlohmann::json find_page_by_id(NotionClient& client, const std::string& database_id,
                                      const std::string& id) {
    nlohmann::json query = {
        {"filter", {{"property", schema::ID}, {"rich_text", {{"equals", id}}}}},
        {"page_size", 1}};
    nlohmann::json res = client.post("/databases/" + database_id + "/query", query);
    if (res.contains("results") && res["results"].is_array() && !res["results"].empty())
        return res["results"][0];
    return nlohmann::json();  // null
}

}  // namespace cmd

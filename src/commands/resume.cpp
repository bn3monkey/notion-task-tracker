#include <iostream>

#include "../page_util.hpp"
#include "command_support.hpp"

namespace {

// Collect plain text of paragraph/heading blocks under a page.
std::string read_body_text(NotionClient& client, const std::string& page_id) {
    std::string out;
    try {
        nlohmann::json res = client.get("/blocks/" + page_id + "/children?page_size=100");
        if (res.contains("results")) {
            for (const auto& b : res["results"]) {
                const std::string type = b.value("type", "");
                if (b.contains(type) && b[type].contains("rich_text")) {
                    std::string line = page_util::plain_text_of(b[type]["rich_text"]);
                    if (!line.empty()) out += line + "\n";
                }
            }
        }
    } catch (...) {
        // body is best-effort; ignore failures
    }
    return out;
}

}  // namespace

int cmd_resume(const Context& ctx, const std::string& id) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;
    if (id.empty()) return cmd::fail(ctx, "이어할 ID를 지정하세요: ntt resume <ID>");

    try {
        NotionClient client(cfg.effective_token());
        nlohmann::json page = cmd::find_page_by_id(client, cfg.database_id, id);
        if (page.is_null()) return cmd::fail(ctx, "ID '" + id + "'에 해당하는 페이지를 찾지 못했습니다.", 3);

        const std::string page_id = page.value("id", "");
        const std::string title = page_util::read_title(page, schema::TITLE);
        const std::string status = page_util::read_select(page, schema::STATUS);
        const std::string priority = page_util::read_select(page, schema::PRIORITY);
        const std::string period_start = page_util::read_date_start(page, schema::PERIOD);
        const std::string period_end = page_util::read_date_end(page, schema::PERIOD);
        const std::string deadline = page_util::read_date_start(page, schema::DEADLINE);
        const std::string body = read_body_text(client, page_id);

        if (ctx.json) {
            nlohmann::json j = {{"ok", true},
                                {"id", id},
                                {"page_id", page_id},
                                {"url", page.value("url", "")},
                                {"title", title},
                                {"status", status},
                                {"priority", priority},
                                {"period_start", period_start},
                                {"period_end", period_end},
                                {"deadline", deadline},
                                {"body", body}};
            cmd::ok_json(j);
        } else {
            std::cout << "[ntt] 이어하기: " << title << " (" << id << ")\n";
            std::cout << "  상태      : " << status << "\n";
            std::cout << "  우선순위  : " << priority << "\n";
            std::cout << "  수행기간  : " << period_start
                      << (period_end.empty() ? "" : " ~ " + period_end) << "\n";
            if (!deadline.empty()) std::cout << "  마감일    : " << deadline << "\n";
            if (!body.empty()) {
                std::cout << "  --- 기존 기록 ---\n";
                std::cout << body;
            }
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

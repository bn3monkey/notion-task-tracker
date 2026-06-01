#include <iostream>

#include "../page_util.hpp"
#include "command_support.hpp"

int cmd_setup(const Context& ctx, const SetupArgs& args) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (args.parent_page_id.empty())
        return cmd::fail(ctx, "--parent <page_id>가 필요합니다 (DB를 생성할 부모 페이지).");

    const std::string title = args.title.empty() ? "업무 트래커" : args.title;

    nlohmann::json body = {
        {"parent", {{"type", "page_id"}, {"page_id", args.parent_page_id}}},
        {"title", nlohmann::json::array({{{"type", "text"}, {"text", {{"content", title}}}}})},
        {"properties", schema::create_properties()}};

    try {
        NotionClient client(cfg.effective_token());
        nlohmann::json db = client.post("/databases", body);
        const std::string db_id = db.value("id", "");

        // Persist the new database id.
        cfg.database_id = db_id;
        cfg.save();

        if (ctx.json) {
            nlohmann::json j = {{"ok", true},
                                {"database_id", db_id},
                                {"url", db.value("url", "")},
                                {"title", title}};
            cmd::ok_json(j);
        } else {
            std::cout << "[ntt] DB 생성 완료: " << title << "\n";
            std::cout << "  database_id: " << db_id << "\n";
            std::cout << "  url        : " << db.value("url", "") << "\n";
            std::cout << "  (config에 저장됨)\n";
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

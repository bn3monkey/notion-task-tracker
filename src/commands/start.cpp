#include <iostream>

#include "../id_gen.hpp"
#include "../page_util.hpp"
#include "../util.hpp"
#include "command_support.hpp"

int cmd_start(const Context& ctx, const StartArgs& args) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;
    if (args.title.empty()) return cmd::fail(ctx, "--title이 필요합니다.");

    try {
        NotionClient client(cfg.effective_token());

        // Generate a unique 6-char id (retry on the rare collision).
        std::string id;
        for (int attempt = 0; attempt < 5; ++attempt) {
            id = generate_id();
            if (cmd::find_page_by_id(client, cfg.database_id, id).is_null()) break;
            id.clear();
        }
        if (id.empty()) return cmd::fail(ctx, "고유 ID 생성 실패 (충돌 반복).");

        const std::string today = util::today_iso();
        const std::string status = args.status.empty() ? schema::default_status() : args.status;
        const std::string priority = args.priority.empty() ? schema::default_priority() : args.priority;

        nlohmann::json props;
        props[schema::TITLE] = page_util::title(args.title);
        props[schema::ID] = page_util::rich_text(id);
        props[schema::PERIOD] = page_util::date(today);
        props[schema::STATUS] = page_util::select(status);
        props[schema::PRIORITY] = page_util::select(priority);
        if (!args.deadline.empty()) props[schema::DEADLINE] = page_util::date(args.deadline);

        nlohmann::json body = {{"parent", {{"database_id", cfg.database_id}}},
                               {"properties", props}};
        nlohmann::json page = client.post("/pages", body);

        if (ctx.json) {
            nlohmann::json j = {{"ok", true},
                                {"id", id},
                                {"page_id", page.value("id", "")},
                                {"url", page.value("url", "")},
                                {"title", args.title},
                                {"status", status},
                                {"priority", priority},
                                {"started", today}};
            cmd::ok_json(j);
        } else {
            std::cout << "[ntt] 트래킹 시작: " << args.title << "\n";
            std::cout << "  식별용 ID : " << id << "   <- 이어하기/종료에 사용\n";
            std::cout << "  상태      : " << status << "\n";
            std::cout << "  url       : " << page.value("url", "") << "\n";
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

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
        schema::Resolved s = cmd::load_schema(client, cfg.database_id);

        if (s.title.empty()) return cmd::fail(ctx, "DB에 title(페이지 제목) 속성이 없습니다.");
        if (s.id.empty())
            return cmd::fail(ctx, "'식별용 ID' 속성이 없습니다. 'ntt setup'으로 누락 필드를 추가하세요.", 2);

        // Generate a unique 6-char id (retry on the rare collision).
        std::string id;
        for (int attempt = 0; attempt < 5; ++attempt) {
            id = generate_id();
            if (cmd::find_page_by_id(client, cfg.database_id, s.id, id).is_null()) break;
            id.clear();
        }
        if (id.empty()) return cmd::fail(ctx, "고유 ID 생성 실패 (충돌 반복).");

        const std::string today = util::today_iso();
        const std::string status = args.status.empty() ? schema::default_status() : args.status;

        nlohmann::json props;
        props[s.title] = page_util::title(args.title);
        props[s.id] = page_util::rich_text(id);
        if (!s.period.empty()) props[s.period] = page_util::date(today);
        if (!s.status.empty()) props[s.status] = page_util::select(status);
        if (!args.priority.empty() && !s.priority.empty())
            props[s.priority] = page_util::select(args.priority);
        if (!args.deadline.empty() && !s.deadline.empty())
            props[s.deadline] = page_util::date(args.deadline);

        nlohmann::json body = {{"parent", {{"database_id", cfg.database_id}}},
                               {"properties", props}};
        nlohmann::json page = client.post("/pages", body);

        if (ctx.json) {
            cmd::ok_json({{"ok", true},
                          {"id", id},
                          {"page_id", page.value("id", "")},
                          {"url", page.value("url", "")},
                          {"title", args.title},
                          {"status", s.status.empty() ? "" : status},
                          {"started", today}});
        } else {
            std::cout << "[ntt] 트래킹 시작: " << args.title << "\n";
            std::cout << "  식별용 ID : " << id << "   <- 이어하기/종료에 사용\n";
            if (!s.status.empty()) std::cout << "  상태      : " << status << "\n";
            std::cout << "  url       : " << page.value("url", "") << "\n";
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

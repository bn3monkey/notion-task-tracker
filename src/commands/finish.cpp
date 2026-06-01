#include <iostream>
#include <sstream>

#include "../page_util.hpp"
#include "../util.hpp"
#include "command_support.hpp"

int cmd_finish(const Context& ctx, const FinishArgs& args) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;
    if (args.id.empty()) return cmd::fail(ctx, "종료할 ID를 지정하세요: ntt finish <ID>");

    std::string summary = args.from_stdin ? util::read_stdin() : args.summary;
    summary = util::trim(summary);

    try {
        NotionClient client(cfg.effective_token());
        nlohmann::json page = cmd::find_page_by_id(client, cfg.database_id, args.id);
        if (page.is_null())
            return cmd::fail(ctx, "ID '" + args.id + "'에 해당하는 페이지를 찾지 못했습니다.", 3);

        const std::string page_id = page.value("id", "");
        const std::string today = util::today_iso();

        // 1) Append the work summary as body blocks (heading + paragraphs per line).
        if (!summary.empty()) {
            nlohmann::json children = nlohmann::json::array();
            children.push_back(page_util::heading("기록 — " + today));
            std::istringstream iss(summary);
            std::string line;
            while (std::getline(iss, line)) {
                line = util::trim(line);
                if (!line.empty()) children.push_back(page_util::paragraph(line));
            }
            client.patch("/blocks/" + page_id + "/children", {{"children", children}});
        }

        // 2) Update properties: period end = today (preserve start), status.
        std::string start = page_util::read_date_start(page, schema::PERIOD);
        if (start.empty()) start = today;
        const std::string status =
            !args.status.empty() ? args.status
                                  : (args.keep_open ? page_util::read_select(page, schema::STATUS)
                                                    : schema::finished_status());

        nlohmann::json props;
        props[schema::PERIOD] = page_util::date(start, today);
        if (!status.empty()) props[schema::STATUS] = page_util::select(status);
        client.patch("/pages/" + page_id, {{"properties", props}});

        if (ctx.json) {
            nlohmann::json j = {{"ok", true},
                                {"id", args.id},
                                {"page_id", page_id},
                                {"url", page.value("url", "")},
                                {"status", status},
                                {"period_end", today},
                                {"appended", !summary.empty()}};
            cmd::ok_json(j);
        } else {
            std::cout << "[ntt] 트래킹 종료: " << args.id << "\n";
            std::cout << "  상태      : " << status << "\n";
            std::cout << "  수행기간  : " << start << " ~ " << today << "\n";
            std::cout << "  기록 추가 : " << (summary.empty() ? "없음" : "완료") << "\n";
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

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

    // Validate keyword inputs up front (no network).
    std::string status_kw_arg;  // normalized --status, if provided
    if (!args.status.empty()) {
        status_kw_arg = schema::normalize_status(args.status);
        if (status_kw_arg.empty())
            return cmd::fail(ctx, "유효하지 않은 진행 상황: '" + args.status +
                                      "'. 사용 가능: " + schema::status_hint());
    }
    std::string priority_norm;  // normalized --priority, if provided
    if (!args.priority.empty()) {
        priority_norm = schema::normalize_priority(args.priority);
        if (priority_norm.empty())
            return cmd::fail(ctx, "유효하지 않은 우선순위: '" + args.priority +
                                      "'. 사용 가능: " + schema::priority_hint());
    }

    try {
        NotionClient client(cfg.effective_token());
        schema::Resolved s = cmd::load_schema(client, cfg.database_id);
        if (s.id.empty())
            return cmd::fail(ctx, "'식별용 ID' 속성이 없습니다. 'ntt setup'을 실행하세요.", 2);

        nlohmann::json page = cmd::find_page_by_id(client, cfg.database_id, s.id, args.id);
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

        // 2) Update properties.
        nlohmann::json props;

        // period: (start entered at `start`) ~ (today). Preserve the original start.
        if (!s.period.empty()) {
            std::string start = page_util::read_date_start(page, s.period);
            if (start.empty()) start = today;
            props[s.period] = page_util::date(start, today);
        }

        // status: --keep-open keeps current; --status sets a specific keyword
        // (작업 완료/보류/검토 대기/장기/폐기 …); default = 작업 완료.
        std::string status;
        if (!s.status.empty()) {
            std::string status_kw;
            if (args.keep_open)
                status_kw = page_util::read_select(page, s.status);  // unchanged
            else if (!status_kw_arg.empty())
                status_kw = status_kw_arg;
            else
                status_kw = schema::finished_status();
            status = schema::reuse_option(s.status_opts, status_kw);
            if (!status.empty() && !args.keep_open) props[s.status] = page_util::select(status);
        }

        // priority: optional update (strict 6 values; existing tag reused).
        std::string priority;
        if (!priority_norm.empty() && !s.priority.empty()) {
            priority = schema::reuse_option(s.priority_opts, priority_norm);
            props[s.priority] = page_util::select(priority);
        }

        // deadline: optional update.
        if (!args.deadline.empty() && !s.deadline.empty())
            props[s.deadline] = page_util::date(args.deadline);

        if (!props.empty()) client.patch("/pages/" + page_id, {{"properties", props}});

        if (ctx.json) {
            cmd::ok_json({{"ok", true},
                          {"id", args.id},
                          {"page_id", page_id},
                          {"url", page.value("url", "")},
                          {"status", status},
                          {"priority", priority},
                          {"deadline", args.deadline},
                          {"period_end", s.period.empty() ? "" : today},
                          {"appended", !summary.empty()}});
        } else {
            std::cout << "[ntt] 트래킹 종료: " << args.id << "\n";
            if (!status.empty()) std::cout << "  상태      : " << status << "\n";
            if (!priority.empty()) std::cout << "  우선순위  : " << priority << "\n";
            if (!args.deadline.empty() && !s.deadline.empty())
                std::cout << "  마감일    : " << args.deadline << "\n";
            if (!s.period.empty()) std::cout << "  종료일    : " << today << "\n";
            std::cout << "  기록 추가 : " << (summary.empty() ? "없음" : "완료") << "\n";
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    } catch (const std::exception& e) {
        return cmd::fail(ctx, std::string("내부 오류: ") + e.what());
    }
}

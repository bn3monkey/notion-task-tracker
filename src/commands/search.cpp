#include <iostream>

#include "../page_util.hpp"
#include "command_support.hpp"

int cmd_search(const Context& ctx, const SearchArgs& args) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;

    try {
        NotionClient client(cfg.effective_token());
        schema::Resolved s = cmd::load_schema(client, cfg.database_id);

        // Build filter: title contains query AND status equals (each optional).
        nlohmann::json conditions = nlohmann::json::array();
        if (!args.query.empty() && !s.title.empty())
            conditions.push_back({{"property", s.title}, {"title", {{"contains", args.query}}}});
        if (!args.status.empty() && !s.status.empty())
            conditions.push_back({{"property", s.status}, {"select", {{"equals", args.status}}}});

        nlohmann::json query;
        query["page_size"] = args.limit > 0 ? args.limit : 10;
        if (conditions.size() == 1)
            query["filter"] = conditions[0];
        else if (conditions.size() > 1)
            query["filter"] = {{"and", conditions}};

        nlohmann::json res = client.post("/databases/" + cfg.database_id + "/query", query);

        nlohmann::json items = nlohmann::json::array();
        if (res.contains("results")) {
            for (const auto& page : res["results"]) {
                items.push_back(
                    {{"id", s.id.empty() ? "" : page_util::read_rich_text(page, s.id)},
                     {"title", s.title.empty() ? "" : page_util::read_title(page, s.title)},
                     {"status", s.status.empty() ? "" : page_util::read_select(page, s.status)},
                     {"page_id", page.value("id", "")},
                     {"url", page.value("url", "")}});
            }
        }

        if (ctx.json) {
            cmd::ok_json({{"ok", true}, {"count", items.size()}, {"results", items}});
        } else if (items.empty()) {
            std::cout << "[ntt] 검색 결과 없음.\n";
        } else {
            std::cout << "[ntt] 검색 결과 (" << items.size() << "):\n";
            for (const auto& it : items) {
                std::cout << "  " << it["id"].get<std::string>() << "  ["
                          << it["status"].get<std::string>() << "]  "
                          << it["title"].get<std::string>() << "\n";
            }
        }
        return 0;
    } catch (const NotionError& e) {
        return cmd::fail(ctx, e.what());
    }
}

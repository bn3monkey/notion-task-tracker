#include <iostream>

#include "../page_util.hpp"
#include "command_support.hpp"

int cmd_search(const Context& ctx, const SearchArgs& args) {
    Config cfg = Config::load();
    if (!cmd::need_token(ctx, cfg)) return 1;
    if (!cmd::need_database(ctx, cfg)) return 1;

    try {
        NotionClient client(cfg.effective_token());

        // Build filter: title contains query AND status equals (each optional).
        nlohmann::json conditions = nlohmann::json::array();
        if (!args.query.empty())
            conditions.push_back({{"property", schema::TITLE}, {"title", {{"contains", args.query}}}});
        if (!args.status.empty())
            conditions.push_back({{"property", schema::STATUS}, {"select", {{"equals", args.status}}}});

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
                items.push_back({{"id", page_util::read_rich_text(page, schema::ID)},
                                 {"title", page_util::read_title(page, schema::TITLE)},
                                 {"status", page_util::read_select(page, schema::STATUS)},
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

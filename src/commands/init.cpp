#include <iostream>

#include "command_support.hpp"

int cmd_init(const Context& ctx, const InitArgs& args) {
    Config cfg = Config::load();
    if (!args.token.empty()) cfg.token = args.token;
    if (!args.database_id.empty()) cfg.database_id = args.database_id;
    cfg.save();

    if (ctx.json) {
        nlohmann::json j = {{"ok", true},
                            {"config_path", Config::path()},
                            {"has_token", !cfg.effective_token().empty()},
                            {"database_id", cfg.database_id}};
        cmd::ok_json(j);
    } else {
        std::cout << "[ntt] 설정 저장: " << Config::path() << "\n";
        std::cout << "  토큰   : " << (cfg.effective_token().empty() ? "(없음)" : "설정됨") << "\n";
        std::cout << "  DB ID  : " << (cfg.database_id.empty() ? "(없음)" : cfg.database_id) << "\n";
    }
    return 0;
}

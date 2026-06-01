#include "config.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using nlohmann::json;

namespace {

std::string env_or_empty(const char* name) {
#if defined(_WIN32)
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, name) == 0 && buf) {
        std::string v(buf);
        free(buf);
        return v;
    }
    return "";
#else
    const char* v = std::getenv(name);
    return v ? std::string(v) : "";
#endif
}

fs::path config_dir() {
#if defined(_WIN32)
    std::string appdata = env_or_empty("APPDATA");
    if (!appdata.empty()) return fs::path(appdata) / "ntt";
    return fs::path(".") / "ntt";
#else
    std::string xdg = env_or_empty("XDG_CONFIG_HOME");
    if (!xdg.empty()) return fs::path(xdg) / "ntt";
    std::string home = env_or_empty("HOME");
    if (!home.empty()) return fs::path(home) / ".config" / "ntt";
    return fs::path(".") / "ntt";
#endif
}

}  // namespace

std::string Config::path() {
    return (config_dir() / "config.json").string();
}

Config Config::load() {
    Config c;
    std::ifstream in(path());
    if (!in) return c;  // no file yet — empty config
    json j;
    try {
        in >> j;
    } catch (...) {
        return c;  // corrupt file — treat as empty
    }
    c.token = j.value("token", "");
    c.database_id = j.value("database_id", "");
    return c;
}

void Config::save() const {
    fs::create_directories(config_dir());
    json j;
    j["token"] = token;
    j["database_id"] = database_id;
    std::ofstream out(path(), std::ios::trunc);
    out << j.dump(2) << "\n";
}

std::string Config::effective_token() const {
    std::string env = env_or_empty("NTT_NOTION_TOKEN");
    return !env.empty() ? env : token;
}

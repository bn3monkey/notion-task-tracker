#pragma once
#include <string>

// Persistent configuration for ntt.
//
// Stored as JSON at:
//   Windows : %APPDATA%\ntt\config.json
//   Unix    : $XDG_CONFIG_HOME/ntt/config.json  (or ~/.config/ntt/config.json)
//
// The Notion token may also be supplied via the NTT_NOTION_TOKEN environment
// variable, which takes precedence over the stored token.
struct Config {
    std::string token;        // Notion integration token (stored)
    std::string database_id;  // task-tracker database id

    // Absolute path to the config file.
    static std::string path();

    // Load config from disk. Missing file yields an empty Config (not an error).
    static Config load();

    // Persist config to disk, creating the directory if needed.
    void save() const;

    // Token actually used for requests: env NTT_NOTION_TOKEN > stored token.
    std::string effective_token() const;
};

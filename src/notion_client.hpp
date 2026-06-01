#pragma once
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

// Thrown on transport errors or Notion API errors (HTTP >= 400).
class NotionError : public std::runtime_error {
public:
    NotionError(long status, std::string message)
        : std::runtime_error(message), status_(status) {}
    long status() const { return status_; }

private:
    long status_;
};

// Minimal Notion REST client over libcurl.
// Base URL: https://api.notion.com/v1
class NotionClient {
public:
    explicit NotionClient(std::string token);

    nlohmann::json get(const std::string& path);
    nlohmann::json post(const std::string& path, const nlohmann::json& body);
    nlohmann::json patch(const std::string& path, const nlohmann::json& body);

    // Must be called once at program start / end.
    static void global_init();
    static void global_cleanup();

private:
    // method: "GET" | "POST" | "PATCH". body may be nullptr.
    nlohmann::json request(const std::string& method, const std::string& path,
                           const nlohmann::json* body);
    std::string token_;
};

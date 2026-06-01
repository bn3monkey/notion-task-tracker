#pragma once
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// task-tracker DB schema: property names, expected Notion types, and helpers
// to (a) build the schema when creating the DB and (b) validate an existing DB.
namespace schema {

// Property display names (Korean — matches the user's existing tracker).
inline constexpr const char* TITLE = "제목";
inline constexpr const char* ID = "식별용 ID";
inline constexpr const char* PERIOD = "수행기간";
inline constexpr const char* DEADLINE = "마감일";
inline constexpr const char* STATUS = "진행 상황";
inline constexpr const char* PRIORITY = "우선 순위";

// Select options.
inline const std::vector<std::string>& status_options() {
    static const std::vector<std::string> v{"시작 전", "진행중", "완료"};
    return v;
}
inline const std::vector<std::string>& priority_options() {
    static const std::vector<std::string> v{"높음", "보통", "낮음"};
    return v;
}
inline const char* default_status() { return "진행중"; }
inline const char* default_priority() { return "보통"; }
inline const char* finished_status() { return "완료"; }

// (property name, expected Notion "type") for every required property.
inline const std::vector<std::pair<std::string, std::string>>& required() {
    static const std::vector<std::pair<std::string, std::string>> v{
        {TITLE, "title"},     {ID, "rich_text"}, {PERIOD, "date"},
        {DEADLINE, "date"},   {STATUS, "select"}, {PRIORITY, "select"},
    };
    return v;
}

// Build the "properties" object for POST /databases.
inline nlohmann::json create_properties() {
    nlohmann::json props;
    props[TITLE] = {{"title", nlohmann::json::object()}};
    props[ID] = {{"rich_text", nlohmann::json::object()}};
    props[PERIOD] = {{"date", nlohmann::json::object()}};
    props[DEADLINE] = {{"date", nlohmann::json::object()}};

    nlohmann::json status_opts = nlohmann::json::array();
    for (const auto& o : status_options()) status_opts.push_back({{"name", o}});
    props[STATUS] = {{"select", {{"options", status_opts}}}};

    nlohmann::json prio_opts = nlohmann::json::array();
    for (const auto& o : priority_options()) prio_opts.push_back({{"name", o}});
    props[PRIORITY] = {{"select", {{"options", prio_opts}}}};
    return props;
}

// Validate a retrieved database JSON (GET /databases/{id}).
// Returns a list of human-readable problems; empty means valid.
inline std::vector<std::string> validate(const nlohmann::json& db) {
    std::vector<std::string> problems;
    if (!db.contains("properties") || !db["properties"].is_object()) {
        problems.push_back("DB에 properties가 없습니다.");
        return problems;
    }
    const auto& props = db["properties"];
    for (const auto& [name, expected_type] : required()) {
        if (!props.contains(name)) {
            problems.push_back("속성 누락: '" + name + "' (" + expected_type + " 필요)");
            continue;
        }
        const std::string actual = props[name].value("type", "");
        if (actual != expected_type) {
            problems.push_back("속성 타입 불일치: '" + name + "' = " + actual +
                               " (기대: " + expected_type + ")");
        }
    }
    return problems;
}

}  // namespace schema

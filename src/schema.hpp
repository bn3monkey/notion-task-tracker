#pragma once
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// task-tracker DB schema.
//
// Property names are matched flexibly against an existing DB:
//   - "제목" is NOT required as a named column — the page's title (the property
//     whose Notion type is "title", regardless of its name) IS the title.
//   - Other fields accept a few alias spellings (e.g. "수행기간" / "수행 기간").
//
// schema::resolve() maps logical keys to the actual property names present in a
// given DB; schema::missing_properties() builds the PATCH body to add absent
// fields; schema::create_properties() builds a full schema for a brand-new DB.
namespace schema {

// Canonical names used when *creating* properties.
inline constexpr const char* TITLE_CANON = "제목";
inline constexpr const char* ID_CANON = "식별용 ID";
inline constexpr const char* PERIOD_CANON = "수행기간";
inline constexpr const char* DEADLINE_CANON = "마감일";
inline constexpr const char* STATUS_CANON = "진행 상황";
inline constexpr const char* PRIORITY_CANON = "우선 순위";

// A non-title field: a logical key, accepted names (first = canonical), type.
struct Field {
    std::string key;                 // "id" | "period" | "deadline" | "status" | "priority"
    std::vector<std::string> names;  // accepted property names
    std::string type;                // Notion property type
};

inline const std::vector<Field>& fields() {
    static const std::vector<Field> v{
        {"id", {ID_CANON}, "rich_text"},
        {"period", {PERIOD_CANON, "수행 기간"}, "date"},
        {"deadline", {DEADLINE_CANON}, "date"},
        {"status", {STATUS_CANON}, "select"},
        {"priority", {PRIORITY_CANON, "우선순위"}, "select"},
    };
    return v;
}

// Select option sets (used when creating/filling the fields).
inline const std::vector<std::string>& status_options() {
    static const std::vector<std::string> v{"작업 예정", "작업 중",   "작업 완료", "작업 보류",
                                             "검토 대기", "장기",      "폐기"};
    return v;
}
inline const std::vector<std::string>& priority_options() {
    static const std::vector<std::string> v{
        "Urgent & Important",  "Urgent & Not Important", "ASAP & Important",
        "ASAP & Not Important", "Someday & Important",   "Someday & Not Important"};
    return v;
}
inline const char* default_status() { return "작업 중"; }    // on `start`
inline const char* finished_status() { return "작업 완료"; }  // on `finish`

// Build a select-property definition with its options.
inline nlohmann::json select_def(const std::vector<std::string>& options) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& o : options) arr.push_back({{"name", o}});
    return {{"select", {{"options", arr}}}};
}

// An empty property definition for the given non-select type.
inline nlohmann::json empty_def(const std::string& type) {
    return {{type, nlohmann::json::object()}};
}

// Name of the title-type property (page name), or "" if none.
inline std::string find_title(const nlohmann::json& db) {
    if (db.contains("properties"))
        for (auto it = db["properties"].begin(); it != db["properties"].end(); ++it)
            if (it.value().value("type", "") == "title") return it.key();
    return "";
}

// Actual property name in `db` matching `f` (alias + type), or "" if absent.
inline std::string resolve_field(const nlohmann::json& db, const Field& f) {
    if (!db.contains("properties")) return "";
    const auto& props = db["properties"];
    for (const auto& name : f.names)
        if (props.contains(name) && props[name].value("type", "") == f.type) return name;
    return "";
}

// Resolved property names for a specific DB.
struct Resolved {
    std::string title, id, period, deadline, status, priority;
};

inline Resolved resolve(const nlohmann::json& db) {
    Resolved r;
    r.title = find_title(db);
    for (const auto& f : fields()) {
        const std::string n = resolve_field(db, f);
        if (f.key == "id") r.id = n;
        else if (f.key == "period") r.period = n;
        else if (f.key == "deadline") r.deadline = n;
        else if (f.key == "status") r.status = n;
        else if (f.key == "priority") r.priority = n;
    }
    return r;
}

// Validate a retrieved DB. Returns human-readable problems (empty = valid).
inline std::vector<std::string> validate(const nlohmann::json& db) {
    std::vector<std::string> problems;
    if (find_title(db).empty()) problems.push_back("title 타입 속성(페이지 제목)이 없습니다.");
    for (const auto& f : fields())
        if (resolve_field(db, f).empty())
            problems.push_back("속성 누락/타입 불일치: '" + f.names[0] + "' (" + f.type + ")");
    return problems;
}

// Properties object for POST /databases (new DB), including a title property.
inline nlohmann::json create_properties() {
    nlohmann::json props;
    props[TITLE_CANON] = {{"title", nlohmann::json::object()}};
    props[ID_CANON] = empty_def("rich_text");
    props[PERIOD_CANON] = empty_def("date");
    props[DEADLINE_CANON] = empty_def("date");
    props[STATUS_CANON] = select_def(status_options());
    props[PRIORITY_CANON] = select_def(priority_options());
    return props;
}

// Build a PATCH /databases/{id} "properties" body adding only the fields that
// are absent from `db`. Returns {properties, names_added}.
inline std::pair<nlohmann::json, std::vector<std::string>> missing_properties(
    const nlohmann::json& db) {
    nlohmann::json props = nlohmann::json::object();
    std::vector<std::string> added;
    for (const auto& f : fields()) {
        if (!resolve_field(db, f).empty()) continue;  // already present
        const std::string& name = f.names[0];
        if (f.type == "select")
            props[name] = select_def(f.key == "status" ? status_options() : priority_options());
        else
            props[name] = empty_def(f.type);
        added.push_back(name);
    }
    return {props, added};
}

}  // namespace schema

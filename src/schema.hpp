#pragma once
#include <algorithm>
#include <cctype>
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

// ----- priority input normalization (strict: only the 6 values) -----
// Accepts a short key (ui/un/ai/an/si/sn) or the full name (case-insensitive).
// Returns the canonical full value, or "" if the input is not valid.
inline std::string normalize_priority(const std::string& raw) {
    std::string s = raw;
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    if (!s.empty()) s.erase(s.find_last_not_of(" \t\r\n") + 1);
    std::string low = s;
    std::transform(low.begin(), low.end(), low.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });

    // 2-letter keys: urgency(u/a/s) + importance(i/n)
    if (low == "ui") return "Urgent & Important";
    if (low == "un") return "Urgent & Not Important";
    if (low == "ai") return "ASAP & Important";
    if (low == "an") return "ASAP & Not Important";
    if (low == "si") return "Someday & Important";
    if (low == "sn") return "Someday & Not Important";

    for (const auto& full : priority_options()) {
        std::string fl = full;
        std::transform(fl.begin(), fl.end(), fl.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (fl == low) return full;
    }
    return "";
}

// Human-readable hint for valid priority inputs (for error messages / help).
inline std::string priority_hint() {
    return "ui=Urgent & Important, un=Urgent & Not Important, "
           "ai=ASAP & Important, an=ASAP & Not Important, "
           "si=Someday & Important, sn=Someday & Not Important";
}

// ----- status input normalization (the 7 진행 상황 keywords) -----
// Accepts the full keyword or a short alias. Returns the canonical keyword,
// or "" if invalid.
inline std::string normalize_status(const std::string& raw) {
    std::string s = raw;
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    if (!s.empty()) s.erase(s.find_last_not_of(" \t\r\n") + 1);

    for (const auto& k : status_options())
        if (k == s) return k;

    // short aliases
    if (s == "예정") return "작업 예정";
    if (s == "중") return "작업 중";
    if (s == "완료") return "작업 완료";
    if (s == "보류") return "작업 보류";
    if (s == "검토" || s == "검토대기") return "검토 대기";
    if (s == "장기") return "장기";
    if (s == "폐기") return "폐기";
    return "";
}

inline std::string status_hint() {
    return "예정/중/완료/보류/검토/장기/폐기 (또는 정식 명칭)";
}

// Read the option names of a select property from a retrieved DB.
inline std::vector<std::string> select_options_of(const nlohmann::json& db,
                                                  const std::string& prop_name) {
    std::vector<std::string> out;
    if (prop_name.empty() || !db.contains("properties")) return out;
    const auto& props = db["properties"];
    if (!props.contains(prop_name)) return out;
    const auto& p = props[prop_name];
    if (p.contains("select") && p["select"].contains("options"))
        for (const auto& o : p["select"]["options"]) out.push_back(o.value("name", ""));
    return out;
}

// Reuse an existing option whose name *contains* `keyword`; otherwise return
// `keyword` itself (Notion will create the option on write).
inline std::string reuse_option(const std::vector<std::string>& existing,
                                const std::string& keyword) {
    for (const auto& o : existing)
        if (!keyword.empty() && o.find(keyword) != std::string::npos) return o;
    return keyword;
}

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

// Resolved property names for a specific DB, plus existing select options.
struct Resolved {
    std::string title, id, period, deadline, status, priority;
    std::vector<std::string> status_opts;    // existing 진행 상황 options
    std::vector<std::string> priority_opts;  // existing 우선 순위 options
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
    r.status_opts = select_options_of(db, r.status);
    r.priority_opts = select_options_of(db, r.priority);
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

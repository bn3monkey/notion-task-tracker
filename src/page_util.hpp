#pragma once
#include <string>

#include <nlohmann/json.hpp>

// Helpers for building Notion property values / blocks and reading them back.
namespace page_util {

// ----- build property values (for POST/PATCH /pages "properties") -----
inline nlohmann::json title(const std::string& text) {
    return {{"title", nlohmann::json::array({{{"text", {{"content", text}}}}})}};
}
inline nlohmann::json rich_text(const std::string& text) {
    return {{"rich_text", nlohmann::json::array({{{"text", {{"content", text}}}}})}};
}
inline nlohmann::json date(const std::string& start, const std::string& end = "") {
    nlohmann::json d = {{"start", start}};
    if (!end.empty()) d["end"] = end;
    return {{"date", d}};
}
inline nlohmann::json select(const std::string& name) {
    return {{"select", {{"name", name}}}};
}

// A paragraph block with a single text run (for appending body content).
inline nlohmann::json paragraph(const std::string& text) {
    return {{"object", "block"},
            {"type", "paragraph"},
            {"paragraph",
             {{"rich_text", nlohmann::json::array({{{"type", "text"}, {"text", {{"content", text}}}}})}}}};
}
// A heading_3 block (used to title an appended work-log section).
inline nlohmann::json heading(const std::string& text) {
    return {{"object", "block"},
            {"type", "heading_3"},
            {"heading_3",
             {{"rich_text", nlohmann::json::array({{{"type", "text"}, {"text", {{"content", text}}}}})}}}};
}

// ----- read property values back (from a retrieved page) -----
inline std::string plain_text_of(const nlohmann::json& rich_array) {
    std::string out;
    if (rich_array.is_array()) {
        for (const auto& r : rich_array) {
            if (r.contains("plain_text"))
                out += r["plain_text"].get<std::string>();
            else if (r.contains("text") && r["text"].contains("content"))
                out += r["text"]["content"].get<std::string>();
        }
    }
    return out;
}

inline const nlohmann::json* prop(const nlohmann::json& page, const std::string& name) {
    if (page.contains("properties") && page["properties"].contains(name))
        return &page["properties"][name];
    return nullptr;
}

inline std::string read_title(const nlohmann::json& page, const std::string& name) {
    const auto* p = prop(page, name);
    if (p && p->contains("title")) return plain_text_of((*p)["title"]);
    return "";
}
inline std::string read_rich_text(const nlohmann::json& page, const std::string& name) {
    const auto* p = prop(page, name);
    if (p && p->contains("rich_text")) return plain_text_of((*p)["rich_text"]);
    return "";
}
inline std::string read_select(const nlohmann::json& page, const std::string& name) {
    const auto* p = prop(page, name);
    if (p && p->contains("select") && (*p)["select"].is_object())
        return (*p)["select"].value("name", "");
    return "";
}
inline std::string read_date_start(const nlohmann::json& page, const std::string& name) {
    const auto* p = prop(page, name);
    if (p && p->contains("date") && (*p)["date"].is_object())
        return (*p)["date"].value("start", "");
    return "";
}
inline std::string read_date_end(const nlohmann::json& page, const std::string& name) {
    const auto* p = prop(page, name);
    if (p && p->contains("date") && (*p)["date"].is_object())
        return (*p)["date"].value("end", "");
    return "";
}

}  // namespace page_util

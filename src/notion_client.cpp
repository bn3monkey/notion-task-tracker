#include "notion_client.hpp"

#include <curl/curl.h>

#include <string>

using nlohmann::json;

namespace {

constexpr const char* kBaseUrl = "https://api.notion.com/v1";
constexpr const char* kNotionVersion = "2022-06-28";

size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

}  // namespace

void NotionClient::global_init() { curl_global_init(CURL_GLOBAL_DEFAULT); }
void NotionClient::global_cleanup() { curl_global_cleanup(); }

NotionClient::NotionClient(std::string token) : token_(std::move(token)) {}

json NotionClient::get(const std::string& path) { return request("GET", path, nullptr); }
json NotionClient::post(const std::string& path, const json& body) {
    return request("POST", path, &body);
}
json NotionClient::patch(const std::string& path, const json& body) {
    return request("PATCH", path, &body);
}

json NotionClient::request(const std::string& method, const std::string& path,
                           const json* body) {
    CURL* curl = curl_easy_init();
    if (!curl) throw NotionError(0, "libcurl 초기화 실패");

    const std::string url = std::string(kBaseUrl) + path;
    std::string response;
    std::string payload = body ? body->dump() : std::string();

    struct curl_slist* headers = nullptr;
    const std::string auth = "Authorization: Bearer " + token_;
    const std::string ver = std::string("Notion-Version: ") + kNotionVersion;
    headers = curl_slist_append(headers, auth.c_str());
    headers = curl_slist_append(headers, ver.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ntt/0.1");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payload.size());
    } else if (method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payload.size());
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        throw NotionError(0, std::string("네트워크 오류: ") + curl_easy_strerror(rc));
    }

    json parsed;
    if (!response.empty()) {
        try {
            parsed = json::parse(response);
        } catch (...) {
            if (status >= 400)
                throw NotionError(status, "HTTP " + std::to_string(status) + " (응답 파싱 실패)");
            throw NotionError(status, "응답 JSON 파싱 실패");
        }
    }

    if (status >= 400) {
        std::string msg = "HTTP " + std::to_string(status);
        if (parsed.contains("message")) msg += ": " + parsed["message"].get<std::string>();
        throw NotionError(status, msg);
    }
    return parsed;
}

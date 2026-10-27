#include "ai/web_search_client.h"

#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

std::string string_field(const nlohmann::json& object, const char* key) {
    const auto it = object.find(key);
    return it != object.end() && it->is_string() ? it->get<std::string>() : std::string();
}

}  // namespace

WebSearchClient::WebSearchClient(const HttpsClient& http, std::string host, std::string api_key)
    : http_(http), host_(std::move(host)), api_key_(std::move(api_key)) {}

std::vector<WebResult> WebSearchClient::search(const std::string& query, int count) const {
    const nlohmann::json request = {{"query", query}, {"count", count}};
    const HttpResponse response = http_.post_json(Utf8::to_wide(host_), L"/v1/tools/search",
                                                  {L"Authorization: Bearer " + Utf8::to_wide(api_key_)},
                                                  request.dump());
    if (response.status != 200) {
        throw TodoError("web search HTTP " + std::to_string(response.status) + ": " + response.body.substr(0, 200));
    }
    const nlohmann::json body = nlohmann::json::parse(response.body, nullptr, false);
    if (body.is_discarded() || !body.contains("results") || !body["results"].is_array()) {
        throw TodoError("web search response has no results array");
    }
    std::vector<WebResult> results;
    for (const nlohmann::json& item : body["results"]) {
        if (!item.is_object()) {
            continue;
        }
        results.push_back(WebResult{string_field(item, "title"), string_field(item, "url"),
                                    string_field(item, "site_name"), string_field(item, "date"),
                                    string_field(item, "summary")});
    }
    return results;
}

}  // namespace todo

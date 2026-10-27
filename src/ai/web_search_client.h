#pragma once

#include <string>
#include <vector>

#include "ai/https_client.h"

namespace todo {

struct WebResult {
    std::string title;
    std::string url;
    std::string site;
    std::string date;     // as returned, may be empty
    std::string summary;  // page excerpt, may be long
};

// DeepSeek web search: POST /v1/tools/search {query, count}, authorized with the DeepSeek API key.
// It returns search results only; the model reads them through WebSearchTool.
class WebSearchClient {
public:
    WebSearchClient(const HttpsClient& http, std::string host, std::string api_key);

    // Throws TodoError on a non-200 response or an unexpected body.
    std::vector<WebResult> search(const std::string& query, int count) const;

private:
    const HttpsClient& http_;
    std::string host_;
    std::string api_key_;
};

}  // namespace todo

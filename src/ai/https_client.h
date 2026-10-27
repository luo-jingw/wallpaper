#pragma once

#include <string>
#include <vector>

namespace todo {

struct HttpResponse {
    int status;
    std::string body;
    long long latency_ms;
};

// Blocking HTTPS POST over WinHTTP.
class HttpsClient {
public:
    HttpResponse post_json(const std::wstring& host, const std::wstring& path, const std::vector<std::wstring>& headers,
                           const std::string& body_utf8) const;
};

}  // namespace todo

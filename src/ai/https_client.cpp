#include "ai/https_client.h"

#include <windows.h>
#include <winhttp.h>

#include <chrono>

#include "util/todo_error.h"

namespace todo {

namespace {

class InternetHandle {
public:
    explicit InternetHandle(HINTERNET handle) : handle_(handle) {}
    ~InternetHandle() {
        if (handle_) {
            WinHttpCloseHandle(handle_);
        }
    }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;

    HINTERNET get() const { return handle_; }

private:
    HINTERNET handle_;
};

[[noreturn]] void fail(const char* step) {
    throw TodoError(std::string("https ") + step + " failed (WinHTTP error " + std::to_string(GetLastError()) + ")");
}

constexpr int kResolveTimeoutMs = 10000;
constexpr int kConnectTimeoutMs = 10000;
constexpr int kSendTimeoutMs = 30000;
constexpr int kReceiveTimeoutMs = 180000;

}  // namespace

HttpResponse HttpsClient::post_json(const std::wstring& host, const std::wstring& path,
                                    const std::vector<std::wstring>& headers, const std::string& body_utf8) const {
    const auto started = std::chrono::steady_clock::now();

    InternetHandle session(WinHttpOpen(L"todo/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                       WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.get()) {
        fail("open");
    }
    WinHttpSetTimeouts(session.get(), kResolveTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs, kReceiveTimeoutMs);

    InternetHandle connection(WinHttpConnect(session.get(), host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection.get()) {
        fail("connect");
    }
    InternetHandle request(WinHttpOpenRequest(connection.get(), L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request.get()) {
        fail("open request");
    }

    std::wstring header_block = L"Content-Type: application/json\r\n";
    for (const std::wstring& header : headers) {
        header_block += header + L"\r\n";
    }
    const BOOL sent = WinHttpSendRequest(request.get(), header_block.c_str(), static_cast<DWORD>(header_block.size()),
                                         const_cast<char*>(body_utf8.data()), static_cast<DWORD>(body_utf8.size()),
                                         static_cast<DWORD>(body_utf8.size()), 0);
    if (!sent) {
        fail("send");
    }
    if (!WinHttpReceiveResponse(request.get(), nullptr)) {
        fail("receive");
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX)) {
        fail("query status");
    }

    std::string body;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            fail("query data");
        }
        if (available == 0) {
            break;
        }
        std::string chunk(available, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), chunk.data(), available, &read)) {
            fail("read");
        }
        body.append(chunk.data(), read);
    }

    const auto elapsed = std::chrono::steady_clock::now() - started;
    return HttpResponse{static_cast<int>(status), body,
                        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()};
}

}  // namespace todo

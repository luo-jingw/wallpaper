#pragma once

#include <string>
#include <vector>

#include "ai/https_client.h"

namespace todo {

struct ToolCall {
    std::string id;
    std::string name;
    std::string arguments_json;
};

struct ChatMessage {
    std::string role;  // system | user | assistant | tool
    std::string content;
    std::vector<ToolCall> tool_calls;  // assistant only
    std::string tool_call_id;          // tool only
};

struct ToolSpec {
    std::string name;
    std::string description;
    std::string parameters_json;  // JSON Schema object
};

struct CompletionOptions {
    std::vector<ToolSpec> tools;  // empty: the request has no tools field
    bool json_output = false;     // response_format json_object; the prompt must mention "json"
};

struct ChatCompletion {
    std::string content;
    std::vector<ToolCall> tool_calls;
    int tokens_in;
    int tokens_out;
    long long latency_ms;
};

// DeepSeek OpenAI-compatible POST /chat/completions, non-streaming.
class DeepSeekClient {
public:
    DeepSeekClient(const HttpsClient& http, std::string host, std::string api_key, std::string model, bool thinking);

    ChatCompletion complete(const std::vector<ChatMessage>& messages, const CompletionOptions& options) const;

private:
    const HttpsClient& http_;
    std::string host_;
    std::string api_key_;
    std::string model_;
    bool thinking_;
};

}  // namespace todo

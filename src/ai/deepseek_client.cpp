#include "ai/deepseek_client.h"

#include <utility>

#include <nlohmann/json.hpp>

#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

nlohmann::json message_json(const ChatMessage& message) {
    nlohmann::json json = {{"role", message.role}, {"content", message.content}};
    if (!message.tool_calls.empty()) {
        json["tool_calls"] = nlohmann::json::array();
        for (const ToolCall& call : message.tool_calls) {
            json["tool_calls"].push_back({
                {"id", call.id},
                {"type", "function"},
                {"function", {{"name", call.name}, {"arguments", call.arguments_json}}},
            });
        }
    }
    if (message.role == "tool") {
        json["tool_call_id"] = message.tool_call_id;
    }
    return json;
}

nlohmann::json tool_json(const ToolSpec& spec) {
    const nlohmann::json parameters = nlohmann::json::parse(spec.parameters_json, nullptr, false);
    if (parameters.is_discarded() || !parameters.is_object()) {
        throw TodoError("tool " + spec.name + " has invalid parameters JSON");
    }
    return {
        {"type", "function"},
        {"function", {{"name", spec.name}, {"description", spec.description}, {"parameters", parameters}}},
    };
}

std::vector<ToolCall> parse_tool_calls(const nlohmann::json& message) {
    std::vector<ToolCall> calls;
    if (!message.contains("tool_calls") || !message["tool_calls"].is_array()) {
        return calls;
    }
    for (const nlohmann::json& call : message["tool_calls"]) {
        const nlohmann::json& function = call.at("function");
        calls.push_back(ToolCall{
            call.at("id").get<std::string>(),
            function.at("name").get<std::string>(),
            function.at("arguments").get<std::string>(),
        });
    }
    return calls;
}

}  // namespace

DeepSeekClient::DeepSeekClient(const HttpsClient& http, std::string host, std::string api_key, std::string model,
                               bool thinking)
    : http_(http), host_(std::move(host)), api_key_(std::move(api_key)), model_(std::move(model)), thinking_(thinking) {}

ChatCompletion DeepSeekClient::complete(const std::vector<ChatMessage>& messages,
                                        const CompletionOptions& options) const {
    nlohmann::json request = {
        {"model", model_},
        {"stream", false},
        {"thinking", {{"type", thinking_ ? "enabled" : "disabled"}}},
        {"messages", nlohmann::json::array()},
    };
    for (const ChatMessage& message : messages) {
        request["messages"].push_back(message_json(message));
    }
    if (!options.tools.empty()) {
        request["tools"] = nlohmann::json::array();
        for (const ToolSpec& spec : options.tools) {
            request["tools"].push_back(tool_json(spec));
        }
    }
    if (options.json_output) {
        request["response_format"] = {{"type", "json_object"}};
    }

    const HttpResponse response = http_.post_json(Utf8::to_wide(host_), L"/chat/completions",
                                                  {L"Authorization: Bearer " + Utf8::to_wide(api_key_)},
                                                  request.dump());

    const nlohmann::json body = nlohmann::json::parse(response.body, nullptr, false);
    if (response.status != 200) {
        std::string detail = Text::single_line(response.body).substr(0, 300);
        if (!body.is_discarded() && body.contains("error") && body["error"].contains("message")) {
            detail = body["error"]["message"].get<std::string>();
        }
        throw TodoError("DeepSeek HTTP " + std::to_string(response.status) + ": " + detail);
    }
    if (body.is_discarded() || !body.contains("choices") || body["choices"].empty()) {
        throw TodoError("DeepSeek response has no choices");
    }
    const nlohmann::json& message = body.at("choices").at(0).at("message");
    const nlohmann::json& usage = body.value("usage", nlohmann::json::object());
    const nlohmann::json& content = message.contains("content") ? message["content"] : nlohmann::json();
    return ChatCompletion{
        content.is_string() ? content.get<std::string>() : std::string(),
        parse_tool_calls(message),
        usage.value("prompt_tokens", 0),
        usage.value("completion_tokens", 0),
        response.latency_ms,
    };
}

}  // namespace todo

// Observes DeepSeek tool calling and JSON output against the real API.
// Usage: deepseek_probe <todo.ini>. Prints requests' shape and measured values; asserts nothing.

#include <fcntl.h>
#include <io.h>

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ai/deepseek_client.h"
#include "ai/https_client.h"
#include "config/app_config.h"
#include "util/utf8.h"

namespace {

void print(const std::string& line) {
    const std::string text = line + "\n";
    std::fwrite(text.data(), 1, text.size(), stdout);
}

void print_completion(const std::string& label, const todo::ChatCompletion& completion) {
    print(label + ": latency_ms=" + std::to_string(completion.latency_ms) + " tokens_in=" +
          std::to_string(completion.tokens_in) + " tokens_out=" + std::to_string(completion.tokens_out) +
          " tool_calls=" + std::to_string(completion.tool_calls.size()));
    for (const todo::ToolCall& call : completion.tool_calls) {
        print("  tool_call id=" + call.id + " name=" + call.name + " arguments=" + call.arguments_json);
    }
    print("  content=" + completion.content);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    _setmode(_fileno(stdout), _O_BINARY);
    if (argc != 2) {
        print("usage: deepseek_probe <todo.ini>");
        return 2;
    }
    try {
        const todo::AppConfig config = todo::AppConfig::load(argv[1]);
        const todo::HttpsClient http;
        const todo::DeepSeekClient client(http, config.api_host(), config.read_api_key(), config.model(),
                                          config.thinking());

        const todo::ToolSpec count_tasks{
            "count_tasks",
            "Count the user's tasks at one level.",
            R"({"type":"object","properties":{"level":{"type":"string","enum":["day","week","long"]}},"required":["level"]})",
        };
        std::vector<todo::ChatMessage> messages = {
            {"system", "You are a task assistant. Use tools when they can answer the question.", {}, {}},
            {"user", "我今天和本周分别有几个任务？", {}, {}},
        };
        const todo::ChatCompletion first = client.complete(messages, todo::CompletionOptions{{count_tasks}, false});
        print_completion("round1", first);

        messages.push_back({"assistant", first.content, first.tool_calls, {}});
        for (const todo::ToolCall& call : first.tool_calls) {
            const nlohmann::json arguments = nlohmann::json::parse(call.arguments_json, nullptr, false);
            const std::string level = arguments.is_object() ? arguments.value("level", std::string()) : "";
            const int count = level == "day" ? 3 : level == "week" ? 5 : 0;
            messages.push_back({"tool", "{\"count\":" + std::to_string(count) + "}", {}, call.id});
            print("  tool_result id=" + call.id + " count=" + std::to_string(count));
        }
        const todo::ChatCompletion second = client.complete(messages, todo::CompletionOptions{{count_tasks}, false});
        print_completion("round2", second);

        const std::vector<todo::ChatMessage> json_messages = {
            {"system",
             "Reply with json only, shaped like {\"blocks\":[{\"start\":\"09:00\",\"end\":\"10:00\",\"title\":\"...\"}]}.",
             {},
             {}},
            {"user", "把上午安排成两个时间块：跑实验、写周报。", {}, {}},
        };
        const todo::ChatCompletion structured = client.complete(json_messages, todo::CompletionOptions{{}, true});
        print_completion("json", structured);
        const nlohmann::json parsed = nlohmann::json::parse(structured.content, nullptr, false);
        print("  json_parsed=" + std::string(parsed.is_discarded() ? "no" : "yes") + " blocks=" +
              std::to_string(!parsed.is_discarded() && parsed.contains("blocks") && parsed["blocks"].is_array()
                                 ? parsed["blocks"].size()
                                 : 0));
    } catch (const std::exception& error) {
        print(std::string("error: ") + error.what());
        return 1;
    }
    return 0;
}

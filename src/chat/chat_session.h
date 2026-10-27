#pragma once

#include <string>
#include <vector>

namespace todo {

struct ToolRecord {
    std::string name;
    std::string arguments_json;
    std::string summary;
    std::string status;  // ok | denied | error
};

struct ChatTurn {
    std::string role;  // user | assistant
    std::string content;
    int tokens_in = 0;  // assistant turns only
    int tokens_out = 0;
    long long latency_ms = 0;
    std::vector<ToolRecord> tools;  // tools used before this assistant reply
};

struct ChatSession {
    std::string id;       // c<YYYYMMDD>-<n>
    std::string title;
    std::string created;  // YYYY-MM-DDTHH:MM
    std::string skill;    // empty: the request has no tools field
    std::vector<ChatTurn> turns;
};

struct ChatSummary {
    std::string id;
    std::string title;
    std::string created;
    int turn_count;
    std::string skill;
};

}  // namespace todo

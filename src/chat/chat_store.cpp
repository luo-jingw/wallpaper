#include "chat/chat_store.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

const std::string kMetaOpen = "<!--";
const std::string kMetaClose = "-->";
const std::string kUserHeading = "## user";
const std::string kAssistantHeading = "## assistant";
const std::string kToolHeading = "## tool";

struct HeadingParts {
    std::string text;
    std::vector<std::pair<std::string, std::string>> metadata;
};

HeadingParts split_heading(const std::string& line, const std::string& location) {
    HeadingParts parts;
    const auto open = line.rfind(kMetaOpen);
    if (open == std::string::npos) {
        parts.text = Text::trim(line);
        return parts;
    }
    const auto close = line.find(kMetaClose, open);
    if (close == std::string::npos) {
        throw TodoError(location + ": unterminated metadata comment");
    }
    parts.text = Text::trim(line.substr(0, open));
    for (const std::string& pair :
         Text::split_whitespace(line.substr(open + kMetaOpen.size(), close - open - kMetaOpen.size()))) {
        const auto equals = pair.find('=');
        if (equals == std::string::npos) {
            throw TodoError(location + ": invalid metadata entry: " + pair);
        }
        parts.metadata.emplace_back(pair.substr(0, equals), pair.substr(equals + 1));
    }
    return parts;
}

std::string join_trimmed(const std::vector<std::string>& lines) {
    size_t begin = 0;
    size_t end = lines.size();
    while (begin < end && Text::trim(lines[begin]).empty()) ++begin;
    while (end > begin && Text::trim(lines[end - 1]).empty()) --end;
    std::string joined;
    for (size_t i = begin; i < end; ++i) {
        if (i > begin) joined += '\n';
        joined += lines[i];
    }
    return joined;
}

bool is_valid_chat_id(const std::string& chat_id) {
    if (chat_id.size() < 11 || chat_id[0] != 'c' || chat_id[9] != '-') {
        return false;
    }
    for (size_t i = 1; i < chat_id.size(); ++i) {
        if (i != 9 && (chat_id[i] < '0' || chat_id[i] > '9')) {
            return false;
        }
    }
    return true;
}

int sequence_of(const std::string& chat_id) {
    return std::stoi(chat_id.substr(10));
}

}  // namespace

ChatStore::ChatStore(std::filesystem::path data_dir, const std::string& folder)
    : chats_dir_(std::move(data_dir) / Utf8::to_path(folder)) {}

ChatSession ChatStore::create(const std::string& title, const Date& today, const std::string& created_at) const {
    const std::string stem = "c" + today.compact() + "-";
    int highest = 0;
    if (std::filesystem::exists(chats_dir_)) {
        for (const auto& entry : std::filesystem::directory_iterator(chats_dir_)) {
            const std::string name = Utf8::from_path(entry.path().stem());
            if (Text::starts_with(name, stem) && is_valid_chat_id(name)) {
                highest = std::max(highest, sequence_of(name));
            }
        }
    }
    ChatSession session;
    session.id = stem + std::to_string(highest + 1);
    session.title = Text::single_line(title);
    session.created = created_at;
    return session;
}

ChatSession ChatStore::load(const std::string& chat_id) const {
    const std::filesystem::path path = file_for(chat_id);
    if (!std::filesystem::exists(path)) {
        throw TodoError("chat not found: " + chat_id);
    }
    return parse(Utf8::read_file(path), Utf8::from_path(path.filename()));
}

void ChatStore::save(const ChatSession& session) const {
    Utf8::write_file(file_for(session.id), serialize(session));
}

std::vector<ChatSummary> ChatStore::list() const {
    std::vector<ChatSummary> summaries;
    if (!std::filesystem::exists(chats_dir_)) {
        return summaries;
    }
    for (const auto& entry : std::filesystem::directory_iterator(chats_dir_)) {
        if (entry.path().extension() != L".md" || !is_valid_chat_id(Utf8::from_path(entry.path().stem()))) {
            continue;
        }
        const ChatSession session = parse(Utf8::read_file(entry.path()), Utf8::from_path(entry.path().filename()));
        summaries.push_back(ChatSummary{session.id, session.title, session.created,
                                        static_cast<int>(session.turns.size()), session.skill});
    }
    std::sort(summaries.begin(), summaries.end(), [](const ChatSummary& a, const ChatSummary& b) {
        if (a.created != b.created) return a.created > b.created;
        return sequence_of(a.id) > sequence_of(b.id);
    });
    return summaries;
}

std::filesystem::path ChatStore::file_for(const std::string& chat_id) const {
    if (!is_valid_chat_id(chat_id)) {
        throw TodoError("invalid chat id: " + chat_id);
    }
    return chats_dir_ / Utf8::to_path(chat_id + ".md");
}

ChatSession ChatStore::parse(const std::string& utf8_text, const std::string& source_name) const {
    ChatSession session;
    std::optional<ChatTurn> current;
    std::optional<ToolRecord> current_tool;
    std::vector<ToolRecord> pending_tools;
    std::vector<std::string> body;
    bool has_header = false;

    const auto flush_tool = [&]() {
        if (!current_tool) {
            return;
        }
        const std::string block = join_trimmed(body);
        const auto split = block.find('\n');
        if (split == std::string::npos) {
            current_tool->summary = block;
        } else {
            current_tool->summary = block.substr(0, split);
            current_tool->arguments_json = Text::trim(block.substr(split + 1));
        }
        pending_tools.push_back(*current_tool);
        current_tool.reset();
        body.clear();
    };

    const auto flush_turn = [&]() {
        flush_tool();
        if (current) {
            current->content = join_trimmed(body);
            if (current->role == "assistant") {
                current->tools = std::move(pending_tools);
                pending_tools.clear();
            }
            session.turns.push_back(*current);
            current.reset();
        }
        body.clear();
    };

    const std::vector<std::string> lines = Text::split_lines(utf8_text);
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        const std::string location = source_name + ":" + std::to_string(i + 1);
        if (!has_header) {
            if (Text::trim(line).empty()) continue;
            if (!Text::starts_with(line, "# ")) {
                throw TodoError(location + ": chat file must start with '# <title>'");
            }
            const HeadingParts parts = split_heading(line.substr(2), location);
            session.title = parts.text;
            for (const auto& [key, value] : parts.metadata) {
                if (key == "id") session.id = value;
                else if (key == "created") session.created = value;
                else if (key == "skill") session.skill = value;
            }
            has_header = true;
            continue;
        }
        if (Text::starts_with(line, kToolHeading)) {
            flush_turn();
            const HeadingParts parts = split_heading(line.substr(3), location);
            ToolRecord record;
            for (const auto& [key, value] : parts.metadata) {
                if (key == "name") record.name = value;
                else if (key == "status") record.status = value;
            }
            current_tool = record;
            continue;
        }
        if (line == kUserHeading || Text::starts_with(line, kAssistantHeading)) {
            flush_turn();
            const HeadingParts parts = split_heading(line.substr(3), location);
            ChatTurn turn;
            turn.role = parts.text;
            for (const auto& [key, value] : parts.metadata) {
                if (key == "tokens_in") turn.tokens_in = std::stoi(value);
                else if (key == "tokens_out") turn.tokens_out = std::stoi(value);
                else if (key == "latency_ms") turn.latency_ms = std::stoll(value);
            }
            current = turn;
            continue;
        }
        body.push_back(line);
    }
    flush_turn();
    if (session.id.empty()) {
        throw TodoError(source_name + ": missing chat id");
    }
    return session;
}

std::string ChatStore::serialize(const ChatSession& session) const {
    std::string out = "# " + session.title + " " + kMetaOpen + " id=" + session.id + " created=" + session.created;
    if (!session.skill.empty()) {
        out += " skill=" + session.skill;
    }
    out += " " + kMetaClose + "\n";
    for (const ChatTurn& turn : session.turns) {
        if (turn.role == "assistant") {
            for (const ToolRecord& record : turn.tools) {
                out += "\n" + kToolHeading + " " + kMetaOpen + " name=" + record.name +
                       " status=" + record.status + " " + kMetaClose + "\n\n";
                out += record.summary;
                if (!record.arguments_json.empty()) {
                    out += "\n" + record.arguments_json;
                }
                out += "\n";
            }
            out += "\n" + kAssistantHeading + " " + kMetaOpen + " tokens_in=" + std::to_string(turn.tokens_in) +
                   " tokens_out=" + std::to_string(turn.tokens_out) + " latency_ms=" +
                   std::to_string(turn.latency_ms) + " " + kMetaClose + "\n\n";
        } else {
            out += "\n" + kUserHeading + "\n\n";
        }
        out += turn.content + "\n";
    }
    return out;
}

}  // namespace todo

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "chat/chat_session.h"
#include "util/date.h"

namespace todo {

// Sole owner of <data_dir>/<folder>/<chat_id>.md: "chats" for Main's conversations, "companion" for the
// mascot's.
// A turn starts at "## user", "## assistant", or "## tool". Tool blocks belong to the following assistant turn.
class ChatStore {
public:
    ChatStore(std::filesystem::path data_dir, const std::string& folder);

    ChatSession create(const std::string& title, const Date& today, const std::string& created_at) const;
    ChatSession load(const std::string& chat_id) const;
    void save(const ChatSession& session) const;
    std::vector<ChatSummary> list() const;  // newest first

private:
    std::filesystem::path file_for(const std::string& chat_id) const;
    ChatSession parse(const std::string& utf8_text, const std::string& source_name) const;
    std::string serialize(const ChatSession& session) const;

    std::filesystem::path chats_dir_;
};

}  // namespace todo

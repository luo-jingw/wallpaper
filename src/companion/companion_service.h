#pragma once

#include <optional>
#include <string>
#include <vector>

#include "chat/chat_service.h"
#include "chat/chat_session.h"
#include "chat/chat_store.h"
#include "util/date.h"

namespace todo {

// The mascot's short conversations: light chat and lookups. It may read everything conversation tools can
// read and search the web, but it cannot change anything. Its tools are fixed here, not by a skill file;
// the caller registers only these tools, and ChatService denies any other call.
class CompanionService {
public:
    // `store` owns <data_dir>/companion/.
    explicit CompanionService(const ChatStore& store);

    static const std::vector<std::string>& read_only_tools();

    // The newest conversation started on `today`, if any.
    std::optional<ChatSession> current(const Date& today) const;
    // A new, saved, empty conversation; later messages today go to it.
    ChatSession start(const Date& today) const;
    // Sends to today's conversation, starting one if there is none. `chat` saves through the same store and
    // has a registry of read_only_tools() only.
    ChatExchange send(const ChatService& chat, const std::string& message, const Date& today,
                      ChatSession& session) const;

private:
    const ChatStore& store_;
};

}  // namespace todo

#pragma once

#include <stdexcept>
#include <string>

namespace todo {

// Every recoverable failure is reported through this type; main converts it into an ERR record.
class TodoError : public std::runtime_error {
public:
    explicit TodoError(const std::string& message_utf8) : std::runtime_error(message_utf8) {}
};

}  // namespace todo

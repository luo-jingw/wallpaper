#pragma once

#include <string>

namespace todo {

enum class ToolStatus { Ok, Denied, Error };

class ToolStatusCodec {
public:
    static std::string to_name(ToolStatus status);
};

}  // namespace todo

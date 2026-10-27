#pragma once

#include "cli/command.h"

namespace todo {

class CommandParser {
public:
    Command parse(int argc, const wchar_t* const* argv) const;
};

}  // namespace todo

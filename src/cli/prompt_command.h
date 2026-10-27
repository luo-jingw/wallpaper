#pragma once

#include "cli/command.h"
#include "cli/command_context.h"

namespace todo {

// prompt <caption> --anchor ... --offset-x N --offset-y N --color-* ... [--dock-below]: the input window.
// Collects text only; it does not touch the content repository.
class PromptCommand {
public:
    explicit PromptCommand(const CommandContext& context) : context_(context) {}

    void run(const Command& command) const;

private:
    const CommandContext& context_;
};

}  // namespace todo

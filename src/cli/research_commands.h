#pragma once

#include "cli/command.h"
#include "cli/command_context.h"

namespace todo {

// research new <title> | research list.
class ResearchCommands {
public:
    explicit ResearchCommands(const CommandContext& context) : context_(context) {}

    void run(const Command& command) const;

private:
    void create(const Command& command) const;
    void list() const;

    const CommandContext& context_;
};

}  // namespace todo

#pragma once

#include "cli/command.h"
#include "cli/command_context.h"

namespace todo {

// list, add, status, note, place, estimate, sync: direct changes to task files, one commit each.
class TaskCommands {
public:
    explicit TaskCommands(const CommandContext& context) : context_(context) {}

    void list(const Command& command) const;
    void add(const Command& command) const;
    void status(const Command& command) const;
    void note(const Command& command) const;
    void place(const Command& command) const;
    void estimate(const Command& command) const;
    void sync() const;

private:
    const CommandContext& context_;
};

}  // namespace todo

#pragma once

#include <string>
#include <vector>

#include "domain/task.h"

namespace todo {

struct TaskFile {
    std::string heading;
    std::vector<Task> tasks;
};

// Converts between a task Markdown file and TaskFile.
// Accepted lines: "# heading", "- [m] title <!-- k=v ... -->", "  > note", blank lines.
// Any other non-blank line is rejected so that hand edits never get silently dropped.
class TaskFileCodec {
public:
    TaskFile parse(const std::string& utf8_text, const std::string& source_name) const;
    std::string serialize(const TaskFile& file) const;
};

}  // namespace todo

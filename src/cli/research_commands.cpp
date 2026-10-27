#include "cli/research_commands.h"

#include <string>
#include <vector>

#include "research/research_store.h"
#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

void ResearchCommands::run(const Command& command) const {
    const std::string& action = context_.require_positional(command, 0, "research action");
    if (action == "new") {
        create(command);
    } else if (action == "list") {
        list();
    } else {
        throw TodoError("research action must be one of new, list");
    }
}

void ResearchCommands::create(const Command& command) const {
    const std::string title = Text::single_line(context_.require_positional(command, 1, "title"));
    if (title.empty()) {
        throw TodoError("title is empty");
    }
    const ResearchNote note = context_.research.create(title, context_.today);
    context_.output.research_note(note);
    context_.commit("research new: " + title);
    context_.output.ok("created " + Utf8::from_path(note.path.filename()));
}

void ResearchCommands::list() const {
    const std::vector<ResearchNote> notes = context_.research.list();
    for (const ResearchNote& note : notes) {
        context_.output.research_note(note);
    }
    context_.output.ok(std::to_string(notes.size()) + " notes");
}

}  // namespace todo

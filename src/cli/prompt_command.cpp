#include "cli/prompt_command.h"

#include <string>

#include "prompt/prompt_theme.h"
#include "prompt/prompt_window.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

void PromptCommand::run(const Command& command) const {
    const std::string& caption = context_.require_positional(command, 0, "caption");
    if (!command.anchor || !command.offset_x || !command.offset_y) {
        throw TodoError("prompt requires --anchor, --offset-x, and --offset-y");
    }
    if (!command.color_bg || !command.color_panel || !command.color_text || !command.color_dim ||
        !command.color_accent) {
        throw TodoError("prompt requires --color-bg, --color-panel, --color-text, --color-dim, and --color-accent");
    }
    const PromptTheme theme = PromptTheme::from_skin_colors(*command.color_bg, *command.color_panel,
                                                            *command.color_text, *command.color_dim,
                                                            *command.color_accent);
    const PromptRequest request{Utf8::to_wide(caption), Utf8::to_wide(*command.anchor), *command.offset_x,
                                *command.offset_y, theme, command.dock_below};
    const PromptResult result = PromptWindow().run(request);
    if (result.text) {
        context_.output.input(Utf8::from_wide(*result.text));
    } else {
        context_.output.cancel();
    }
    context_.output.position(result.offset_x, result.offset_y);
}

}  // namespace todo

#include "planning/plan_block_codec.h"

#include <array>
#include <optional>
#include <regex>
#include <vector>

#include "tools/tool_arguments.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

const std::array<std::string, 4> kKinds = {"task", "fixed", "commute", "break"};
const std::array<std::string, 5> kPlaces = {"lab", "class", "remote", "any", "transit"};
const std::regex kClock("([01][0-9]|2[0-4]):[0-5][0-9]");
const std::regex kWeekStart("(Mon|Tue|Wed|Thu|Fri|Sat|Sun) ([01][0-9]|2[0-4]):[0-5][0-9]");
const std::regex kTaskId("[dl][0-9]{8}-[0-9]+|w[0-9]{4}W[0-9]{2}-[0-9]+");

template <size_t N>
bool one_of(const std::array<std::string, N>& values, const std::string& value) {
    for (const std::string& candidate : values) {
        if (candidate == value) {
            return true;
        }
    }
    return false;
}

void validate(const PlanBlock& block, bool week) {
    const std::regex& start_pattern = week ? kWeekStart : kClock;
    if (!std::regex_match(block.start, start_pattern)) {
        throw TodoError(std::string("block start must be ") + (week ? "weekday and time such as \"Tue 09:00\"" : "\"HH:MM\"") +
                        ", got: " + block.start);
    }
    if (!std::regex_match(block.end, kClock)) {
        throw TodoError("block end must be \"HH:MM\", got: " + block.end);
    }
    if (!one_of(kKinds, block.kind)) {
        throw TodoError("block kind must be one of task, fixed, commute, break, got: " + block.kind);
    }
    if (!one_of(kPlaces, block.place)) {
        throw TodoError("block place must be one of lab, class, remote, any, transit, got: " + block.place);
    }
    if (block.title.empty()) {
        throw TodoError("block title is empty at " + block.start);
    }
    if (block.task_id && !PlanBlockCodec::is_task_id(*block.task_id)) {
        throw TodoError("block task_id is not a task id: " + *block.task_id);
    }
}

}  // namespace

bool PlanBlockCodec::is_task_id(const std::string& text) {
    return std::regex_match(text, kTaskId);
}

PlanBlock PlanBlockCodec::from_json(const nlohmann::json& object, bool week) {
    if (!object.is_object()) {
        throw TodoError("block is not an object");
    }
    PlanBlock block;
    block.start = ToolArguments::required_string(object, "start");
    block.end = ToolArguments::required_string(object, "end");
    // A week block may repeat its weekday on end ("Tue 12:00"); it must be the start's weekday.
    std::smatch end_match;
    if (week && std::regex_match(block.end, end_match, kWeekStart)) {
        if (block.start.substr(0, 4) != block.end.substr(0, 4)) {
            throw TodoError("a block must end on the day it starts: " + block.start + " - " + block.end);
        }
        block.end = block.end.substr(4);
    }
    block.kind = ToolArguments::required_string(object, "kind");
    block.place = ToolArguments::required_string(object, "place");
    block.title = Text::single_line(ToolArguments::required_string(object, "title"));
    block.task_id = ToolArguments::optional_string(object, "task_id");
    if (block.task_id && block.task_id->empty()) {
        block.task_id.reset();
    }
    validate(block, week);
    return block;
}

std::string PlanBlockCodec::to_line(const PlanBlock& block) {
    std::string line = block.start + "-" + block.end + " " + block.kind + " " + block.place + " " + block.title;
    if (block.task_id) {
        line += " (" + *block.task_id + ")";
    }
    return line;
}

PlanBlock PlanBlockCodec::from_line(const std::string& line, bool week) {
    const std::regex pattern(week ? R"(^(\S+ \S+)-(\S+) (\S+) (\S+) (.+)$)" : R"(^(\S+)-(\S+) (\S+) (\S+) (.+)$)");
    std::smatch match;
    if (!std::regex_match(line, match, pattern)) {
        throw TodoError("invalid block line: " + line);
    }
    PlanBlock block{match[1].str(), match[2].str(), match[3].str(), match[4].str(), Text::trim(match[5].str()),
                    std::nullopt};
    static const std::regex kTrailingId(R"(^(.*) \(([^()\s]+)\)$)");
    std::smatch id_match;
    if (std::regex_match(block.title, id_match, kTrailingId) && is_task_id(id_match[2].str())) {
        block.task_id = id_match[2].str();
        block.title = Text::trim(id_match[1].str());
    }
    validate(block, week);
    return block;
}

}  // namespace todo

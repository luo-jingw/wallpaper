#include "planning/plan_change_codec.h"

#include <optional>
#include <vector>

#include "domain/task.h"
#include "domain/task_level.h"
#include "domain/task_place.h"
#include "planning/plan_block_codec.h"
#include "tools/tool_arguments.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

std::string require_task_id(const nlohmann::json& object) {
    const std::string task_id = ToolArguments::required_string(object, "task_id");
    if (!PlanBlockCodec::is_task_id(task_id)) {
        throw TodoError("invalid task_id: " + task_id);
    }
    return task_id;
}

std::string require_place(const std::optional<std::string>& name) {
    const std::string value = name.value_or("any");
    if (!TaskPlaceCodec::from_name(value)) {
        throw TodoError("place must be one of any, lab, class, remote, got: " + value);
    }
    return value;
}

}  // namespace

PlanChange PlanChangeCodec::from_json(int number, const nlohmann::json& object) {
    if (!object.is_object()) {
        throw TodoError("change " + std::to_string(number) + " is not an object");
    }
    const std::string op = ToolArguments::required_string(object, "op");
    PlanChange change{number, op, nlohmann::json::object(), ""};
    if (op == "set_place") {
        const std::string task_id = require_task_id(object);
        const std::string place = require_place(ToolArguments::required_string(object, "place"));
        change.arguments = {{"task_id", task_id}, {"place", place}};
        change.description = "set_place " + task_id + " " + place;
    } else if (op == "set_estimate") {
        const std::string task_id = require_task_id(object);
        const std::optional<int> minutes = ToolArguments::optional_int(object, "estimate_minutes", 1);
        if (!minutes) {
            throw TodoError("set_estimate needs estimate_minutes");
        }
        change.arguments = {{"task_id", task_id}, {"estimate_minutes", *minutes}};
        change.description = "set_estimate " + task_id + " " + std::to_string(*minutes);
    } else if (op == "add_task") {
        const std::string level = ToolArguments::required_string(object, "level");
        if (!TaskLevelCodec::from_name(level)) {
            throw TodoError("level must be one of day, week, long, got: " + level);
        }
        const std::string title = Text::single_line(ToolArguments::required_string(object, "title"));
        if (title.empty()) {
            throw TodoError("add_task title is empty");
        }
        const std::string place = require_place(ToolArguments::optional_string(object, "place"));
        const std::optional<int> minutes = ToolArguments::optional_int(object, "estimate_minutes", 1);
        change.arguments = {{"level", level}, {"title", title}, {"place", place}};
        if (minutes) {
            change.arguments["estimate_minutes"] = *minutes;
        }
        change.description =
            "add_task " + level + " " + place + " " + (minutes ? std::to_string(*minutes) : "-") + " " + title;
    } else {
        throw TodoError("change op must be one of add_task, set_place, set_estimate, got: " + op);
    }
    return change;
}

PlanChange PlanChangeCodec::from_line(int number, const std::string& description) {
    const std::vector<std::string> words = Text::split_whitespace(description);
    if (words.empty()) {
        throw TodoError("empty change " + std::to_string(number));
    }
    nlohmann::json object = {{"op", words[0]}};
    if (words[0] == "set_place" && words.size() == 3) {
        object["task_id"] = words[1];
        object["place"] = words[2];
    } else if (words[0] == "set_estimate" && words.size() == 3) {
        const std::optional<int> minutes = Text::parse_positive_int(words[2]);
        if (!minutes) {
            throw TodoError("invalid estimate in change: " + description);
        }
        object["task_id"] = words[1];
        object["estimate_minutes"] = *minutes;
    } else if (words[0] == "add_task" && words.size() >= 5) {
        object["level"] = words[1];
        object["place"] = words[2];
        if (words[3] != "-") {
            const std::optional<int> minutes = Text::parse_positive_int(words[3]);
            if (!minutes) {
                throw TodoError("invalid estimate in change: " + description);
            }
            object["estimate_minutes"] = *minutes;
        }
        // The title is everything after the fourth word, spacing preserved.
        size_t position = 0;
        for (int i = 0; i < 4; ++i) {
            position = description.find(words[static_cast<size_t>(i)], position) + words[static_cast<size_t>(i)].size();
        }
        object["title"] = Text::trim(description.substr(position));
    } else {
        throw TodoError("invalid change line: " + description);
    }
    return from_json(number, object);
}

}  // namespace todo

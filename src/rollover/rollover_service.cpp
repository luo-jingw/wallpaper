#include "rollover/rollover_service.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "domain/task_status.h"
#include "storage/task_file.h"
#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

const std::string kStateKey = "last_rollover: ";

}  // namespace

RolloverService::RolloverService(TaskRepository& repository, std::filesystem::path state_file)
    : repository_(repository), state_file_(std::move(state_file)) {}

RolloverResult RolloverService::ensure(const Date& today) {
    const std::optional<Date> last = read_state();
    if (!last) {
        write_state(today);
        return RolloverResult{today, today, 0, 0, true};
    }
    if (*last >= today) {
        return RolloverResult{*last, today, 0, 0, false};
    }
    const int day_moved = move_unfinished(TaskLevel::Day, *last, today);
    const int week_moved =
        last->iso_week() == today.iso_week() ? 0 : move_unfinished(TaskLevel::Week, *last, today);
    write_state(today);
    return RolloverResult{*last, today, day_moved, week_moved, true};
}

int RolloverService::move_unfinished(TaskLevel level, const Date& from, const Date& to) {
    if (!repository_.exists(level, from)) {
        return 0;
    }
    TaskFile source = repository_.load(level, from);
    TaskFile target = repository_.load(level, to);
    int moved = 0;
    for (Task& task : source.tasks) {
        if (!TaskStatusCodec::is_unfinished(task.status)) {
            continue;
        }
        if (task.id.empty()) {
            task.id = repository_.allocate_id(level, from, source);
        }
        if (task.created.empty()) {
            task.created = from.to_string();
        }
        const bool already_present = std::any_of(target.tasks.begin(), target.tasks.end(),
                                                 [&](const Task& existing) { return existing.id == task.id; });
        if (!already_present) {
            Task copy = task;
            copy.rolled += 1;
            target.tasks.push_back(std::move(copy));
        }
        task.status = TaskStatus::Rolled;
        ++moved;
    }
    if (moved > 0) {
        repository_.save(level, from, source);
        repository_.save(level, to, target);
    }
    return moved;
}

std::optional<Date> RolloverService::read_state() const {
    if (!std::filesystem::exists(state_file_)) {
        return std::nullopt;
    }
    for (const std::string& line : Text::split_lines(Utf8::read_file(state_file_))) {
        if (Text::starts_with(line, kStateKey)) {
            const std::optional<Date> date = Date::parse(Text::trim(line.substr(kStateKey.size())));
            if (!date) {
                throw TodoError("invalid date in " + Utf8::from_path(state_file_));
            }
            return date;
        }
    }
    throw TodoError("missing '" + kStateKey + "' in " + Utf8::from_path(state_file_));
}

void RolloverService::write_state(const Date& date) const {
    Utf8::write_file(state_file_, "# Rollover\n\n" + kStateKey + date.to_string() + "\n");
}

}  // namespace todo

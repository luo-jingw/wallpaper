#include "storage/task_repository.h"

#include <algorithm>
#include <utility>

#include "domain/task_level.h"
#include "util/text.h"
#include "util/utf8.h"

namespace todo {

TaskRepository::TaskRepository(std::filesystem::path data_dir) : tasks_dir_(std::move(data_dir) / "tasks") {}

std::filesystem::path TaskRepository::file_for(TaskLevel level, const Date& date) const {
    switch (level) {
        case TaskLevel::Day: return tasks_dir_ / "days" / (date.to_string() + ".md");
        case TaskLevel::Week: return tasks_dir_ / "weeks" / (date.iso_week().to_string() + ".md");
        case TaskLevel::LongTerm: return tasks_dir_ / "long_term.md";
    }
    return tasks_dir_ / "long_term.md";
}

bool TaskRepository::exists(TaskLevel level, const Date& date) const {
    return std::filesystem::exists(file_for(level, date));
}

TaskFile TaskRepository::load(TaskLevel level, const Date& date) const {
    const std::filesystem::path path = file_for(level, date);
    if (!std::filesystem::exists(path)) {
        return TaskFile{heading_for(level, date), {}};
    }
    TaskFile file = codec_.parse(Utf8::read_file(path), Utf8::from_path(path.filename()));
    if (file.heading.empty()) {
        file.heading = heading_for(level, date);
    }
    return file;
}

void TaskRepository::save(TaskLevel level, const Date& date, TaskFile& file) const {
    for (Task& task : file.tasks) {
        if (task.id.empty()) {
            task.id = allocate_id(level, date, file);
        }
        if (task.created.empty()) {
            task.created = date.to_string();
        }
    }
    Utf8::write_file(file_for(level, date), codec_.serialize(file));
}

std::string TaskRepository::allocate_id(TaskLevel level, const Date& date, const TaskFile& file) const {
    const std::string stem = std::string(1, TaskLevelCodec::id_prefix(level)) + period_for(level, date) + "-";
    int highest = 0;
    for (const Task& task : file.tasks) {
        if (Text::starts_with(task.id, stem)) {
            highest = std::max(highest, std::stoi(task.id.substr(stem.size())));
        }
    }
    return stem + std::to_string(highest + 1);
}

std::optional<TaskLocation> TaskRepository::find(const std::string& task_id, const Date& today) const {
    for (const TaskLevel level : {TaskLevel::Day, TaskLevel::Week, TaskLevel::LongTerm}) {
        TaskFile file = load(level, today);
        for (size_t i = 0; i < file.tasks.size(); ++i) {
            if (file.tasks[i].id == task_id) {
                return TaskLocation{level, today, std::move(file), i};
            }
        }
    }
    return std::nullopt;
}

std::string TaskRepository::heading_for(TaskLevel level, const Date& date) const {
    switch (level) {
        case TaskLevel::Day: return date.to_string();
        case TaskLevel::Week: return date.iso_week().to_string();
        case TaskLevel::LongTerm: return "Long Term";
    }
    return {};
}

std::string TaskRepository::period_for(TaskLevel level, const Date& date) const {
    switch (level) {
        case TaskLevel::Day: return date.compact();
        case TaskLevel::Week: {
            const IsoWeek week = date.iso_week();
            std::string period = week.to_string();
            period.erase(std::remove(period.begin(), period.end(), '-'), period.end());
            return period;
        }
        case TaskLevel::LongTerm: return date.compact();
    }
    return {};
}

}  // namespace todo

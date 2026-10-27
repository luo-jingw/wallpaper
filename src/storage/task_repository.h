#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include "domain/task.h"
#include "storage/task_file.h"
#include "util/date.h"

namespace todo {

struct TaskLocation {
    TaskLevel level;
    Date date;  // any date inside the file's period
    TaskFile file;
    size_t index;
};

// Sole owner of the task files under <data_dir>/tasks.
class TaskRepository {
public:
    explicit TaskRepository(std::filesystem::path data_dir);

    std::filesystem::path file_for(TaskLevel level, const Date& date) const;
    bool exists(TaskLevel level, const Date& date) const;
    TaskFile load(TaskLevel level, const Date& date) const;
    // Tasks without an id (hand-written lines) receive one before the file is written.
    void save(TaskLevel level, const Date& date, TaskFile& file) const;
    std::string allocate_id(TaskLevel level, const Date& date, const TaskFile& file) const;
    // Searches the current day file, the current week file, and the long-term file.
    std::optional<TaskLocation> find(const std::string& task_id, const Date& today) const;

private:
    std::string heading_for(TaskLevel level, const Date& date) const;
    std::string period_for(TaskLevel level, const Date& date) const;

    std::filesystem::path tasks_dir_;
    TaskFileCodec codec_;
};

}  // namespace todo

#pragma once

#include <filesystem>
#include <optional>

#include "domain/task.h"
#include "storage/task_repository.h"
#include "util/date.h"

namespace todo {

struct RolloverResult {
    Date from;
    Date to;
    int day_tasks_moved;
    int week_tasks_moved;
    bool performed;  // true when any content file was written
};

// Sole owner of <data_dir>/tasks/rollover_state.md (the date of the most recent rollover).
class RolloverService {
public:
    RolloverService(TaskRepository& repository, std::filesystem::path state_file);

    // Idempotent: a second call with the same date writes nothing.
    RolloverResult ensure(const Date& today);

private:
    int move_unfinished(TaskLevel level, const Date& from, const Date& to);
    std::optional<Date> read_state() const;
    void write_state(const Date& date) const;

    TaskRepository& repository_;
    std::filesystem::path state_file_;
};

}  // namespace todo

#include "planning/plan_profile.h"

#include <utility>

#include "util/utf8.h"

namespace todo {

namespace {

const std::string kPlacesExample =
    "# 地点与通勤\n"
    "\n"
    "- lab：实验室。\n"
    "- class：上课的教学楼，与实验室相邻，步行约 5 分钟，可以视为同一地点（校园）。\n"
    "- remote：校外远程办公的地方。与校园之间单程约 1 小时，往返约 2 小时。\n"
    "- any：任何地点都能做的事。\n"
    "\n"
    "- 一天之内尽量只在 remote 与校园之间往返一次。\n"
    "- 通勤时间可以做 any 类型的轻量任务，例如读论文、回消息。\n";

const std::string kScheduleExample =
    "# 每周固定安排\n"
    "\n"
    "（示例，请按实际情况修改。）\n"
    "\n"
    "- 作息：09:00 开始，18:30 结束；午饭 12:00-13:00。\n"
    "- 周二、周四 10:00-11:40：上课（class）。\n"
    "- 周三 14:00-15:30：组会（lab）。\n"
    "- 周一、周五：默认 remote，除非当天有必须在 lab 做的事。\n"
    "- 周末：不排固定工作。\n";

}  // namespace

PlanProfile::PlanProfile(std::filesystem::path data_dir)
    : places_file_(data_dir / "profile" / "places.md"), schedule_file_(std::move(data_dir) / "profile" / "schedule.md") {}

bool PlanProfile::ensure_examples() const {
    bool written = false;
    if (!std::filesystem::exists(places_file_)) {
        Utf8::write_file(places_file_, kPlacesExample);
        written = true;
    }
    if (!std::filesystem::exists(schedule_file_)) {
        Utf8::write_file(schedule_file_, kScheduleExample);
        written = true;
    }
    return written;
}

PlanProfileText PlanProfile::read() const {
    return PlanProfileText{Utf8::read_file(places_file_), Utf8::read_file(schedule_file_)};
}

}  // namespace todo

#include "planning/plan_format.h"

namespace todo {

namespace {

const std::string kDayExample =
    R"({"blocks":[)"
    R"({"start":"10:00","end":"11:00","kind":"commute","place":"transit","title":"remote -> campus","task_id":null},)"
    R"({"start":"11:30","end":"13:00","kind":"fixed","place":"class","title":"Course","task_id":null},)"
    R"({"start":"13:30","end":"15:00","kind":"task","place":"lab","title":"Run experiment B","task_id":"d20261020-1"}],)"
    R"("changes":[)"
    R"({"op":"set_place","task_id":"d20261020-3","place":"remote"},)"
    R"({"op":"set_estimate","task_id":"d20261020-3","estimate_minutes":45},)"
    R"({"op":"add_task","level":"day","title":"Write meeting notes","place":"lab","estimate_minutes":30}],)"
    R"("rationale":"..."})";

const std::string kWeekExample =
    R"({"blocks":[)"
    R"({"start":"Mon 09:00","end":"12:00","kind":"task","place":"remote","title":"Write chapter 3","task_id":"w2026W43-1"},)"
    R"({"start":"Tue 09:00","end":"10:00","kind":"commute","place":"transit","title":"remote -> campus","task_id":null},)"
    R"({"start":"Tue 14:00","end":"15:30","kind":"fixed","place":"class","title":"Course","task_id":null}],)"
    R"("changes":[)"
    R"({"op":"set_place","task_id":"w2026W43-2","place":"lab"},)"
    R"({"op":"add_task","level":"week","title":"Prepare group meeting slides","place":"any","estimate_minutes":60}],)"
    R"("rationale":"..."})";

const std::string kRules =
    "Rules:\n"
    "- blocks are in chronological order and do not overlap. end is \"HH:MM\" on the same day as start.\n"
    "- kind is task (work on a task), fixed (a fixed appointment from the weekly schedule), commute (travel "
    "between remote and campus), or break.\n"
    "- place is lab, class, remote, any, or transit (commute blocks only). lab and class are both on campus.\n"
    "- task_id is the id of the task the block works on, copied exactly from the task list, or null.\n"
    "- A commute block between remote and campus takes the time given in the places file. Moving between lab "
    "and class needs no commute block.\n"
    "- Never schedule anything before the current time. Statements the user made in the conversation (for "
    "example when they leave home) override the defaults.\n"
    "- A task whose title or latest note names a weekday or a date is scheduled only on that day, and only "
    "if that day is inside the plan.\n"
    "- Group work by place to keep the number of commutes low. Respect the fixed schedule. Skip tasks that "
    "are done or dropped.\n"
    "- changes only use add_task, set_place, or set_estimate, refer to existing task ids, change something "
    "(never repeat the current value), and only appear when they help the plan. changes may be empty.\n"
    "- Write titles and rationale in the language of the tasks. rationale is 2-4 sentences.\n";

}  // namespace

std::string PlanFormat::example(bool week) {
    return week ? kWeekExample : kDayExample;
}

std::string PlanFormat::time_formats() {
    return "Times: a day plan block has start \"HH:MM\" and end \"HH:MM\". A week plan block has start "
           "\"Tue 09:00\" (weekday Mon Tue Wed Thu Fri Sat Sun, a space, the time) and end \"HH:MM\" on the same "
           "day, for example {\"start\":\"Tue 09:00\",\"end\":\"10:00\",\"kind\":\"commute\",\"place\":\"transit\","
           "\"title\":\"remote -> campus\",\"task_id\":null}.\n";
}

std::string PlanFormat::rules(bool week) {
    return std::string(week ? "For a week plan, start is \"Mon HH:MM\" (Mon Tue Wed Thu Fri Sat Sun).\n"
                            : "For a day plan, start is \"HH:MM\".\n") +
           kRules;
}

}  // namespace todo

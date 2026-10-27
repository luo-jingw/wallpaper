#include "planning/planner.h"

#include <optional>
#include <utility>
#include <vector>

#include "domain/task_level.h"
#include "domain/task_text.h"
#include "planning/plan_format.h"
#include "planning/plan_parser.h"
#include "planning/plan_period.h"
#include "skills/skill.h"
#include "storage/task_file.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

const std::string kPlannerSkill = "planner";
constexpr size_t kConversationTurns = 12;
constexpr size_t kTurnCodePoints = 800;

}  // namespace

Planner::Planner(const DeepSeekClient& client, const TaskRepository& tasks, const SkillStore& skills,
                 const PlanProfile& profile)
    : client_(client), tasks_(tasks), skills_(skills), profile_(profile) {}

PlanProposal Planner::propose(TaskLevel scope, const Date& date, const PlanRequestContext& context) const {
    const std::string period = PlanPeriod::of(scope, date);
    const Skill skill = skills_.load(kPlannerSkill);
    const std::optional<Date> now_date = Date::parse(context.now.substr(0, 10));
    if (!now_date) {
        throw TodoError("invalid current time: " + context.now);
    }
    const std::string system = skill.system_prompt + "\nNow is " + now_date->to_string() + " " +
                               context.now.substr(11) + " (" + now_date->weekday_name() + ", " +
                               now_date->iso_week().to_string() + "; " + DateTime::local_zone_label() +
                               "). Times the user gives are in this time zone.";
    const std::string user = request_text(scope, date, context);
    const std::vector<ChatMessage> messages = {ChatMessage{"system", system, {}, {}},
                                               ChatMessage{"user", user, {}, {}}};
    CompletionOptions options;
    options.json_output = true;
    const ChatCompletion completion = client_.complete(messages, options);
    Plan plan = PlanParser().parse(completion.content, period, context.now);
    return PlanProposal{std::move(plan), completion, system.size() + user.size()};
}

std::string Planner::request_text(TaskLevel scope, const Date& date, const PlanRequestContext& context) const {
    const std::string today = context.now.substr(0, 10);
    const std::string clock = context.now.substr(11);
    std::string text;
    if (scope == TaskLevel::Day) {
        text = "Make a day plan for " + date.to_string() + " (" + date.weekday_name() + ").\n";
        if (date.to_string() == today) {
            text += "That is today. The current time is " + clock + "; the first block starts at " + clock +
                    " or later.\n";
        }
    } else {
        const Date monday = date.add_days(1 - date.weekday_iso());
        text = "Make a week plan for " + date.iso_week().to_string() + " (Monday " + monday.to_string() +
               " to Sunday " + monday.add_days(6).to_string() + "). Plan from " + date.weekday_name() + " " +
               date.to_string() + " to the end of the week.\n";
        if (date.to_string() == today) {
            text += "Today's blocks start at " + clock + " or later; earlier days of the week are over.\n";
        }
    }

    text += "\n# Tasks\n";
    for (const TaskLevel level : {TaskLevel::Day, TaskLevel::Week, TaskLevel::LongTerm}) {
        const TaskFile file = tasks_.load(level, date);
        text += "\n## " + TaskLevelCodec::to_name(level) + " (" + file.heading + ")\n";
        for (const Task& task : file.tasks) {
            text += TaskText::describe(task);
        }
    }

    const PlanProfileText profile = profile_.read();
    text += "\n# Places and commute (profile/places.md)\n\n" + profile.places;
    text += "\n# Weekly fixed schedule (profile/schedule.md)\n\n" + profile.schedule;

    if (!context.conversation.empty()) {
        text += "\n# Recent conversation with the user\n\n";
        const size_t first =
            context.conversation.size() > kConversationTurns ? context.conversation.size() - kConversationTurns : 0;
        for (size_t i = first; i < context.conversation.size(); ++i) {
            const ChatTurn& turn = context.conversation[i];
            text += turn.role + ": " +
                    Text::truncate_code_points(Text::single_line(turn.content), kTurnCodePoints) + "\n";
        }
    }

    const bool week = scope == TaskLevel::Week;
    text += "\n# Output\n\nReply with one json object shaped like this example:\n";
    text += PlanFormat::example(week) + "\n\n" + PlanFormat::rules(week) + "- Reply with the json object only.\n";
    return text;
}

}  // namespace todo

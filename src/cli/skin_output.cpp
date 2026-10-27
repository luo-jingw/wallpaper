#include "cli/skin_output.h"

#include <fcntl.h>
#include <io.h>

#include <algorithm>
#include <cstdio>

#include "domain/task_place.h"
#include "domain/task_status.h"
#include "util/utf8.h"

namespace todo {

SkinOutput::SkinOutput() {
    _setmode(_fileno(stdout), _O_BINARY);
}

void SkinOutput::config(const std::string& key, const std::string& value) {
    record("CONFIG", {key, value});
}

void SkinOutput::task(const Task& task) {
    record("TASK", {task.id, TaskStatusCodec::to_name(task.status), task.title, task.created,
                    std::to_string(task.rolled), task.parent.value_or(""),
                    task.notes.empty() ? "" : task.notes.back(), TaskPlaceCodec::to_name(task.place),
                    task.estimate_minutes ? std::to_string(*task.estimate_minutes) : ""});
}

void SkinOutput::rollover(const RolloverResult& result) {
    record("ROLLOVER", {result.from.to_string(), result.to.to_string(), std::to_string(result.day_tasks_moved),
                        std::to_string(result.week_tasks_moved), result.performed ? "yes" : "no"});
}

void SkinOutput::research_note(const ResearchNote& note) {
    record("NOTE", {note.date, note.title, Utf8::from_path(note.path)});
}

void SkinOutput::chat(const ChatSummary& summary) {
    record("CHAT", {summary.id, summary.title, summary.created, std::to_string(summary.turn_count),
                    summary.skill});
}

void SkinOutput::turn(const ChatTurn& turn) {
    record("TURN", {turn.role, turn.content});
}

void SkinOutput::skill(const Skill& skill) {
    std::string tools;
    for (const std::string& name : skill.tools) {
        if (!tools.empty()) {
            tools += ',';
        }
        tools += name;
    }
    record("SKILL", {skill.name, skill.title, tools});
}

void SkinOutput::tool(const ToolRecord& record_data) {
    record("TOOL", {record_data.name, record_data.summary, record_data.status});
}

void SkinOutput::usage(const ChatExchange& exchange) {
    record("USAGE", {std::to_string(exchange.reply.tokens_in), std::to_string(exchange.reply.tokens_out),
                     std::to_string(exchange.reply.latency_ms), std::to_string(exchange.request_messages),
                     std::to_string(exchange.system_chars)});
}

void SkinOutput::usage(const PlanProposal& proposal) {
    record("USAGE", {std::to_string(proposal.completion.tokens_in), std::to_string(proposal.completion.tokens_out),
                     std::to_string(proposal.completion.latency_ms), "2", std::to_string(proposal.prompt_chars)});
}

void SkinOutput::plan(const Plan& plan, const std::map<std::string, std::string>& task_titles) {
    record("PLAN", {plan.period, plan.status, plan.created});
    for (const PlanBlock& block : plan.blocks) {
        record("BLOCK", {block.start, block.end, block.kind, block.place, block.title, block.task_id.value_or("")});
    }
    for (const PlanChange& change : plan.changes) {
        const bool applied = std::find(plan.applied.begin(), plan.applied.end(), change.number) != plan.applied.end();
        std::string title;
        if (change.arguments.contains("task_id")) {
            const auto found = task_titles.find(change.arguments["task_id"].get<std::string>());
            if (found != task_titles.end()) {
                title = found->second;
            }
        }
        record("CHANGE", {std::to_string(change.number), change.description, applied ? "applied" : "", title});
    }
    record("RATIONALE", {plan.rationale});
}

void SkinOutput::input(const std::string& text) {
    record("INPUT", {text});
}

void SkinOutput::cancel() {
    record("CANCEL", {});
}

void SkinOutput::position(int offset_x, int offset_y) {
    record("POSITION", {std::to_string(offset_x), std::to_string(offset_y)});
}

void SkinOutput::commit(const std::optional<std::string>& sha, const std::string& message) {
    record("COMMIT", {sha.value_or("none"), message});
}

void SkinOutput::ok(const std::string& message) {
    record("OK", {message});
}

void SkinOutput::error(const std::string& message) {
    record("ERR", {message});
}

void SkinOutput::record(const std::string& type, const std::vector<std::string>& fields) {
    std::string line = type;
    for (const std::string& field : fields) {
        line += '\t';
        line += escape(field);
    }
    line += '\n';
    std::fwrite(line.data(), 1, line.size(), stdout);
    std::fflush(stdout);
}

std::string SkinOutput::escape(const std::string& field) {
    std::string escaped;
    escaped.reserve(field.size());
    for (const char c : field) {
        switch (c) {
            case '\\': escaped += "\\\\"; break;
            case '\t': escaped += "\\t"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

}  // namespace todo

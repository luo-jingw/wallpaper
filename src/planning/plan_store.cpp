#include "planning/plan_store.h"

#include <utility>
#include <vector>

#include "planning/plan_block_codec.h"
#include "planning/plan_change_codec.h"
#include "planning/plan_period.h"
#include "util/date.h"
#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

const std::string kBlocksHeading = "## Blocks";
const std::string kChangesHeading = "## Changes";
const std::string kRationaleHeading = "## Rationale";

void require_valid_period(const std::string& period) {
    if (!Date::parse(period) && !PlanPeriod::is_week(period)) {
        throw TodoError("period must be YYYY-MM-DD or YYYY-Www, got: " + period);
    }
}

std::string join_numbers(const std::vector<int>& numbers) {
    std::string text;
    for (const int number : numbers) {
        if (!text.empty()) {
            text += ',';
        }
        text += std::to_string(number);
    }
    return text;
}

std::vector<int> split_numbers(const std::string& text, const std::string& location) {
    std::vector<int> numbers;
    std::string item;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == ',') {
            const std::optional<int> number = Text::parse_positive_int(item);
            if (!number) {
                throw TodoError(location + ": invalid applied list: " + text);
            }
            numbers.push_back(*number);
            item.clear();
        } else {
            item += text[i];
        }
    }
    return numbers;
}

}  // namespace

PlanStore::PlanStore(std::filesystem::path data_dir) : plans_dir_(std::move(data_dir) / "plans") {}

std::filesystem::path PlanStore::path_for(const std::string& period) const {
    require_valid_period(period);
    return plans_dir_ / Utf8::to_path(period + ".md");
}

void PlanStore::save(const Plan& plan) const {
    Utf8::write_file(path_for(plan.period), serialize(plan));
}

std::optional<Plan> PlanStore::load(const std::string& period) const {
    const std::filesystem::path path = path_for(period);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }
    return parse(period, Utf8::read_file(path));
}

std::string PlanStore::serialize(const Plan& plan) const {
    std::string text = "# Plan " + plan.period + " <!-- created=" + plan.created + " status=" + plan.status;
    if (!plan.applied.empty()) {
        text += " applied=" + join_numbers(plan.applied);
    }
    text += " -->\n\n" + kBlocksHeading + "\n";
    for (const PlanBlock& block : plan.blocks) {
        text += "- " + PlanBlockCodec::to_line(block) + "\n";
    }
    text += "\n" + kChangesHeading + "\n";
    for (const PlanChange& change : plan.changes) {
        text += "- [" + std::to_string(change.number) + "] " + change.description + "\n";
    }
    text += "\n" + kRationaleHeading + "\n" + plan.rationale + "\n";
    return text;
}

Plan PlanStore::parse(const std::string& period, const std::string& utf8_text) const {
    const std::string location = "plans/" + period + ".md";
    const bool week = PlanPeriod::is_week(period);
    const std::vector<std::string> lines = Text::split_lines(utf8_text);
    size_t i = 0;
    while (i < lines.size() && Text::trim(lines[i]).empty()) {
        ++i;
    }
    const std::string title = "# Plan " + period + " <!--";
    if (i >= lines.size() || !Text::starts_with(lines[i], title)) {
        throw TodoError(location + ": first line must be '" + title + " ... -->'");
    }
    Plan plan;
    plan.period = period;
    const std::string& heading = lines[i];
    const auto close = heading.find("-->", title.size());
    if (close == std::string::npos) {
        throw TodoError(location + ": unterminated metadata comment");
    }
    for (const std::string& pair : Text::split_whitespace(heading.substr(title.size(), close - title.size()))) {
        const auto equals = pair.find('=');
        if (equals == std::string::npos) {
            throw TodoError(location + ": invalid metadata entry: " + pair);
        }
        const std::string key = pair.substr(0, equals);
        const std::string value = pair.substr(equals + 1);
        if (key == "created") {
            plan.created = value;
        } else if (key == "status") {
            plan.status = value;
        } else if (key == "applied") {
            plan.applied = split_numbers(value, location);
        } else {
            throw TodoError(location + ": unknown metadata key: " + key);
        }
    }
    if (plan.status != "proposed" && plan.status != "applied" && plan.status != "discarded") {
        throw TodoError(location + ": status must be proposed, applied, or discarded");
    }

    std::string section;
    std::vector<std::string> rationale;
    for (++i; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        // The rationale is free text and runs to the end of the file.
        if (section == kRationaleHeading) {
            rationale.push_back(line);
            continue;
        }
        if (line == kBlocksHeading || line == kChangesHeading || line == kRationaleHeading) {
            section = line;
            continue;
        }
        if (Text::trim(line).empty()) {
            continue;
        }
        if (!Text::starts_with(line, "- ")) {
            throw TodoError(location + ": unexpected line: " + line);
        }
        if (section == kBlocksHeading) {
            plan.blocks.push_back(PlanBlockCodec::from_line(line.substr(2), week));
        } else if (section == kChangesHeading) {
            const auto bracket = line.find("] ");
            if (line.size() < 4 || line[2] != '[' || bracket == std::string::npos) {
                throw TodoError(location + ": change line must be '- [n] ...': " + line);
            }
            const std::optional<int> number = Text::parse_positive_int(line.substr(3, bracket - 3));
            if (!number) {
                throw TodoError(location + ": invalid change number: " + line);
            }
            plan.changes.push_back(PlanChangeCodec::from_line(*number, line.substr(bracket + 2)));
        } else {
            throw TodoError(location + ": line outside a section: " + line);
        }
    }
    std::string joined;
    for (const std::string& line : rationale) {
        if (!joined.empty()) {
            joined += '\n';
        }
        joined += line;
    }
    plan.rationale = Text::trim(joined);
    return plan;
}

}  // namespace todo

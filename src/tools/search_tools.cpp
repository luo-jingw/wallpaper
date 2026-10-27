#include "tools/search_tools.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

#include "tools/tool_arguments.h"
#include "util/date.h"
#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

constexpr size_t kMaxHits = 40;
constexpr size_t kMaxHitsPerFile = 5;
constexpr size_t kMaxLineCodePoints = 160;
constexpr std::uintmax_t kMaxSearchedFileBytes = 512 * 1024;
constexpr int kDefaultWebResults = 5;
constexpr int kMaxWebResults = 8;
constexpr size_t kMaxSummaryCodePoints = 400;

// ASCII letters only: the rest of UTF-8 (Chinese included) matches exactly.
std::string lower_ascii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool searchable(const std::filesystem::path& path) {
    const std::string extension = lower_ascii(Utf8::from_path(path.extension()));
    return extension == ".md" || extension == ".txt";
}

std::string display_path(const std::filesystem::path& path, const std::filesystem::path& data_dir) {
    std::error_code error;
    const std::filesystem::path relative = std::filesystem::relative(path, data_dir, error);
    if (!error && !relative.empty() && Utf8::from_path(relative).rfind("..", 0) != 0) {
        return Utf8::from_path(relative.generic_wstring());
    }
    return Utf8::from_path(path);
}

}  // namespace

ToolSpec SearchFilesTool::spec() const {
    return ToolSpec{
        "search_files",
        "Search the user's notes, tasks, plans, and conversations (.md and .txt files) for a text. Matching is "
        "case-insensitive for Latin letters. Returns up to 40 lines as path:line: text.",
        R"({"type":"object","properties":{"query":{"type":"string","description":"Text to find"},)"
        R"("path":{"type":"string","description":"Directory to search, relative to the content repository; default: all of it"}},)"
        R"("required":["query"]})",
    };
}

ToolOutcome SearchFilesTool::execute(const nlohmann::json& arguments) const {
    const std::string query = Text::trim(ToolArguments::required_string(arguments, "query"));
    if (query.empty()) {
        throw TodoError("query is empty");
    }
    const std::string requested = ToolArguments::optional_string(arguments, "path").value_or(".");
    const AccessDecision decision = policy_.check(requested, AccessKind::Read);
    if (!decision.allowed) {
        return ToolOutcome{ToolStatus::Denied, "denied: " + decision.reason, false, decision.reason};
    }
    const std::string needle = lower_ascii(query);
    std::vector<std::string> hits;
    std::error_code error;
    std::filesystem::recursive_directory_iterator it(
        decision.resolved, std::filesystem::directory_options::skip_permission_denied, error);
    if (error) {
        return ToolOutcome{ToolStatus::Error, "error: not a directory: " + requested, false, "not a directory"};
    }
    for (; it != std::filesystem::recursive_directory_iterator() && hits.size() < kMaxHits; it.increment(error)) {
        if (error) {
            break;
        }
        const std::filesystem::path& path = it->path();
        if (it->is_directory(error) && path.filename() == L".git") {
            it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file(error) || !searchable(path) || it->file_size(error) > kMaxSearchedFileBytes) {
            continue;
        }
        const std::string text = Utf8::read_file(path);
        if (!Utf8::is_valid(text)) {
            continue;
        }
        const std::vector<std::string> lines = Text::split_lines(text);
        size_t in_file = 0;
        for (size_t i = 0; i < lines.size() && in_file < kMaxHitsPerFile && hits.size() < kMaxHits; ++i) {
            if (lower_ascii(lines[i]).find(needle) != std::string::npos) {
                hits.push_back(display_path(path, policy_.data_dir()) + ":" + std::to_string(i + 1) + ": " +
                               Text::truncate_code_points(Text::trim(lines[i]), kMaxLineCodePoints));
                ++in_file;
            }
        }
    }
    std::string result = hits.empty() ? "no matches for: " + query : "";
    for (const std::string& hit : hits) {
        result += hit + "\n";
    }
    return ToolOutcome{ToolStatus::Ok, result, false, std::to_string(hits.size()) + " matches for " + query};
}

ToolSpec WebSearchTool::spec() const {
    return ToolSpec{
        "web_search",
        "Search the web. Returns titles, links, dates, and excerpts; cite the links you use.",
        R"({"type":"object","properties":{"query":{"type":"string","description":"Search query"},)"
        R"("count":{"type":"integer","description":"Number of results, 1-8, default 5"}},"required":["query"]})",
    };
}

ToolOutcome WebSearchTool::execute(const nlohmann::json& arguments) const {
    const std::string query = Text::trim(ToolArguments::required_string(arguments, "query"));
    if (query.empty()) {
        throw TodoError("query is empty");
    }
    const int count = std::min(ToolArguments::optional_int(arguments, "count", 1).value_or(kDefaultWebResults),
                               kMaxWebResults);
    const std::vector<WebResult> results = client_.search(query, count);
    std::string text = results.empty() ? "no results for: " + query : "";
    int index = 0;
    for (const WebResult& result : results) {
        text += "[" + std::to_string(++index) + "] " + result.title + "\n" + result.url + "\n";
        if (!result.date.empty()) {
            // Dates come with their UTC offset; shown as the local date.
            const std::string date = DateTime::local_date_of(result.date).value_or(result.date.substr(0, 10));
            text += date + (result.site.empty() ? "" : "  " + result.site) + "\n";
        }
        text += Text::truncate_code_points(Text::single_line(result.summary), kMaxSummaryCodePoints) + "\n\n";
    }
    return ToolOutcome{ToolStatus::Ok, text, false, std::to_string(results.size()) + " web results for " + query};
}

}  // namespace todo

#include "research/research_store.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>

#include "util/text.h"
#include "util/utf8.h"

namespace todo {

namespace {

constexpr size_t kMaxSlugCodePoints = 40;
constexpr std::string_view kForbiddenFileNameChars = "\\/:*?\"<>|";

std::string trim_dashes(const std::string& text) {
    const auto begin = text.find_first_not_of('-');
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = text.find_last_not_of('-');
    return text.substr(begin, end - begin + 1);
}

}  // namespace

ResearchStore::ResearchStore(std::filesystem::path data_dir) : research_dir_(std::move(data_dir) / "research") {}

ResearchNote ResearchStore::create(const std::string& title, const Date& today) const {
    const std::string slug = slug_for(title);
    const std::string stem = slug.empty() ? today.to_string() : today.to_string() + "-" + slug;
    std::filesystem::path path = research_dir_ / Utf8::to_path(stem + ".md");
    for (int suffix = 2; std::filesystem::exists(path); ++suffix) {
        path = research_dir_ / Utf8::to_path(stem + "-" + std::to_string(suffix) + ".md");
    }
    Utf8::write_file(path, "# " + title + "\n\n<!-- created=" + today.to_string() + " -->\n\n");
    return ResearchNote{path, today.to_string(), title};
}

std::vector<ResearchNote> ResearchStore::list() const {
    std::vector<std::filesystem::path> paths;
    if (std::filesystem::exists(research_dir_)) {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(research_dir_)) {
            if (entry.is_regular_file() && entry.path().extension() == L".md") {
                paths.push_back(entry.path());
            }
        }
    }
    std::sort(paths.begin(), paths.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b) {
                  return a.filename().wstring() > b.filename().wstring();
              });

    std::vector<ResearchNote> notes;
    for (const std::filesystem::path& path : paths) {
        notes.push_back(read(path));
    }
    return notes;
}

std::string ResearchStore::slug_for(const std::string& title) {
    std::string replaced;
    for (const char c : title) {
        const unsigned char byte = static_cast<unsigned char>(c);
        const bool unsafe = byte < 0x20 || c == ' ' || kForbiddenFileNameChars.find(c) != std::string_view::npos;
        const char kept = unsafe ? '-' : c;
        if (kept == '-' && !replaced.empty() && replaced.back() == '-') {
            continue;
        }
        replaced += kept;
    }
    return trim_dashes(Text::truncate_code_points(trim_dashes(replaced), kMaxSlugCodePoints));
}

ResearchNote ResearchStore::read(const std::filesystem::path& path) const {
    const std::vector<std::string> lines = Text::split_lines(Utf8::read_file(path));
    std::string title = lines.empty() ? std::string() : lines.front();
    if (Text::starts_with(title, "# ")) {
        title = title.substr(2);
    }
    const std::string date = Utf8::from_path(path.filename()).substr(0, 10);
    return ResearchNote{path, date, Text::trim(title)};
}

}  // namespace todo

// Feeds a model reply to PlanParser and prints the parsed plan or the error. Writes nothing.
// Usage: plan_probe <period> <json>
#include <windows.h>

#include <cstdio>
#include <exception>
#include <string>

#include "planning/plan.h"
#include "planning/plan_block_codec.h"
#include "planning/plan_parser.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace {

void print(const std::string& line) {
    const std::string text = line + "\n";
    std::fwrite(text.data(), 1, text.size(), stdout);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        print("usage: plan_probe <period> <json>");
        return 2;
    }
    try {
        const todo::Plan plan =
            todo::PlanParser().parse(todo::Utf8::from_wide(argv[2]), todo::Utf8::from_wide(argv[1]), "probe");
        print("PLAN\t" + plan.period + "\t" + plan.status + "\tblocks=" + std::to_string(plan.blocks.size()) +
              "\tchanges=" + std::to_string(plan.changes.size()));
        for (const todo::PlanBlock& block : plan.blocks) {
            print("BLOCK\t" + todo::PlanBlockCodec::to_line(block));
        }
        for (const todo::PlanChange& change : plan.changes) {
            print("CHANGE\t" + std::to_string(change.number) + "\t" + change.description);
        }
        return 0;
    } catch (const todo::TodoError& error) {
        print(std::string("ERR\t") + error.what());
        return 1;
    } catch (const std::exception& error) {
        print(std::string("ERR\tinternal error: ") + error.what());
        return 2;
    }
}

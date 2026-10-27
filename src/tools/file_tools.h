#pragma once

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "ai/deepseek_client.h"
#include "prompt/confirm_window.h"
#include "tools/path_policy.h"
#include "tools/tool.h"

namespace todo {

// File tools ask PathPolicy before every access and ConfirmWindow before writes it flags.
struct FileToolContext {
    const PathPolicy& policy;
    const ConfirmWindow& confirm;
    std::wstring anchor_title;  // skin window the confirmation is placed on; may be empty
};

class ListDirTool : public Tool {
public:
    explicit ListDirTool(FileToolContext context) : context_(std::move(context)) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    FileToolContext context_;
};

class ReadFileTool : public Tool {
public:
    explicit ReadFileTool(FileToolContext context) : context_(std::move(context)) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    FileToolContext context_;
};

class WriteFileTool : public Tool {
public:
    explicit WriteFileTool(FileToolContext context) : context_(std::move(context)) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    FileToolContext context_;
};

class ReplaceInFileTool : public Tool {
public:
    explicit ReplaceInFileTool(FileToolContext context) : context_(std::move(context)) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    FileToolContext context_;
};

}  // namespace todo

cmake_minimum_required(VERSION 3.20)
project(todo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(todo
    src/main.cpp
    src/util/utf8.cpp
    src/util/date.cpp
    src/util/text.cpp
    src/config/app_config.cpp
    src/domain/task_place.cpp
    src/domain/task_status.cpp
    src/domain/task_level.cpp
    src/domain/task_text.cpp
    src/storage/task_file.cpp
    src/storage/task_repository.cpp
    src/rollover/rollover_service.cpp
    src/process/process_runner.cpp
    src/vcs/git_client.cpp
    src/research/research_store.cpp
    src/ai/https_client.cpp
    src/ai/deepseek_client.cpp
    src/chat/chat_store.cpp
    src/chat/chat_service.cpp
    src/companion/companion_service.cpp
    src/ai/web_search_client.cpp
    src/tools/search_tools.cpp
    src/skills/default_skills.cpp
    src/skills/skill_store.cpp
    src/cli/command_parser.cpp
    src/cli/command_runner.cpp
    src/cli/command_context.cpp
    src/cli/conversation_tools.cpp
    src/cli/task_commands.cpp
    src/cli/research_commands.cpp
    src/cli/conversation_commands.cpp
    src/cli/companion_commands.cpp
    src/cli/plan_commands.cpp
    src/cli/prompt_command.cpp
    src/cli/skin_output.cpp
    src/prompt/prompt_window.cpp
    src/prompt/prompt_theme.cpp
    src/prompt/window_support.cpp
    src/prompt/confirm_window.cpp
    src/tools/tool_status.cpp
    src/tools/tool_arguments.cpp
    src/tools/tool_registry.cpp
    src/tools/path_policy.cpp
    src/tools/task_tools.cpp
    src/tools/file_tools.cpp
    src/planning/plan_period.cpp
    src/planning/plan_block_codec.cpp
    src/planning/plan_change_codec.cpp
    src/planning/plan_parser.cpp
    src/planning/plan_store.cpp
    src/planning/plan_profile.cpp
    src/planning/planner.cpp
    src/planning/plan_applier.cpp
    src/planning/plan_format.cpp
    src/tools/plan_tools.cpp
)

target_include_directories(todo PRIVATE src)
# nlohmann/json 3.11.3 single header, SHA256 9BEA4C8066EF4A1C206B2BE5A36302F8926F7FDC6087AF5D20B417D0CF103EA6
target_include_directories(todo SYSTEM PRIVATE third_party)
target_compile_definitions(todo PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(todo PRIVATE /utf-8 /W4 /permissive-)
target_link_libraries(todo PRIVATE winhttp user32 gdi32 dwmapi)

add_executable(deepseek_probe
    probes/deepseek_probe.cpp
    src/util/utf8.cpp
    src/util/text.cpp
    src/config/app_config.cpp
    src/ai/https_client.cpp
    src/ai/deepseek_client.cpp
)
target_include_directories(deepseek_probe PRIVATE src)
target_include_directories(deepseek_probe SYSTEM PRIVATE third_party)
target_compile_definitions(deepseek_probe PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(deepseek_probe PRIVATE /utf-8 /W4 /permissive-)
target_link_libraries(deepseek_probe PRIVATE winhttp)

add_executable(tools_probe
    probes/tools_probe.cpp
    src/util/utf8.cpp
    src/util/text.cpp
    src/util/date.cpp
    src/domain/task_place.cpp
    src/domain/task_status.cpp
    src/domain/task_level.cpp
    src/domain/task_text.cpp
    src/storage/task_file.cpp
    src/storage/task_repository.cpp
    src/prompt/window_support.cpp
    src/prompt/confirm_window.cpp
    src/tools/tool_status.cpp
    src/tools/tool_arguments.cpp
    src/tools/tool_registry.cpp
    src/tools/path_policy.cpp
    src/tools/task_tools.cpp
    src/tools/file_tools.cpp
)
target_include_directories(tools_probe PRIVATE src)
target_include_directories(tools_probe SYSTEM PRIVATE third_party)
target_compile_definitions(tools_probe PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(tools_probe PRIVATE /utf-8 /W4 /permissive-)
target_link_libraries(tools_probe PRIVATE user32 gdi32)

add_executable(plan_probe
    probes/plan_probe.cpp
    src/util/utf8.cpp
    src/util/text.cpp
    src/util/date.cpp
    src/domain/task_place.cpp
    src/domain/task_level.cpp
    src/tools/tool_arguments.cpp
    src/planning/plan_period.cpp
    src/planning/plan_block_codec.cpp
    src/planning/plan_change_codec.cpp
    src/planning/plan_parser.cpp
)
target_include_directories(plan_probe PRIVATE src)
target_include_directories(plan_probe SYSTEM PRIVATE third_party)
target_compile_definitions(plan_probe PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(plan_probe PRIVATE /utf-8 /W4 /permissive-)

# Rainmeter plugin for the mascot's eyes; installed into %APPDATA%\Rainmeter\Plugins by scripts/install_skin.ps1.
add_library(CursorPos SHARED plugins/cursor_pos/cursor_pos.cpp)
target_compile_definitions(CursorPos PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(CursorPos PRIVATE /utf-8 /W4 /permissive-)
target_link_libraries(CursorPos PRIVATE user32)

# Rainmeter plugin for the mascot's sleep and the ToDo window's position; installed with CursorPos.
add_library(DesktopState SHARED plugins/desktop_state/desktop_state.cpp)
target_compile_definitions(DesktopState PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(DesktopState PRIVATE /utf-8 /W4 /permissive-)
target_link_libraries(DesktopState PRIVATE user32)

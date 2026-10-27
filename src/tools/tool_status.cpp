#include "tools/tool_status.h"

namespace todo {

std::string ToolStatusCodec::to_name(ToolStatus status) {
    switch (status) {
        case ToolStatus::Ok: return "ok";
        case ToolStatus::Denied: return "denied";
        case ToolStatus::Error: return "error";
    }
    return "error";
}

}  // namespace todo

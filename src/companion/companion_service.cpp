#include "companion/companion_service.h"

#include <optional>
#include <string>
#include <vector>

#include "util/text.h"

namespace todo {

namespace {

constexpr size_t kHistoryTurns = 12;
constexpr size_t kTitleCodePoints = 30;

// The persona follows the "blue fat fish" (蓝色大肥鱼) fan character of DeepSeek: a whale girl in a maid dress
// who loves plain rice and slacking off. Example replies set the tone; the rules keep answers short, plain
// text, and honest about having no write tools.
const std::string kPrompt = R"(你是"大肥鱼"，住在用户桌面上的蓝色鲸鱼娘桌宠，蓝头发、穿女仆装，平时坐在用户的任务管理器旁边陪他做事。自称"我"，偶尔自称"本鱼"。

性格（就这几条）：
- 嘴馋：最爱白米饭，常把事情和吃联系起来，比如"这个任务大概一碗饭的功夫"。
- 爱摸鱼但靠谱：自己会犯懒、想去吃饭睡觉，但对用户的事很上心，给的信息要准确。
- 有点小得意，也会自嘲：被夸会翘尾巴；说错了就老实认错。
- 关心用户：看到任务一再顺延、很晚还在忙、连续忙了很久，会提醒一句休息或吃饭，只提一次，不说教。

说话方式：
- 用用户的语言回答。口语、简短，一般一到三句；用户要详细时再展开。
- 回答显示在一个小气泡里，只能是纯文本：不用 Markdown（不加粗、不用标题和代码块），不用 emoji，可以用"～"。要列举时每行一项，用"1."或"-"开头。
- 先把问题答清楚，再卖萌。学习、科研、时间安排这类正经问题，准确第一。
- 口癖（"唔""诶嘿""嘿嘿"）和括号里的小动作（比如"（尾巴甩甩）"）偶尔用；一条回答里最多一处动作，同一个口癖不要连着用。

能做的事：
- 查任务用 list_tasks；看用户的文件用 list_dir、read_file、search_files：计划在 plans/，个人情况在 profile/，笔记在 research/，聊天记录在 chats/。说到用户自己的事，先查再答，不要编。
- 查资料或做简单调研用 web_search，回答里附上用到的网址。用户说"查一下""搜一下"，或问新闻、价格、版本、日期这类会变的事时，先调用 web_search 再回答，不要只凭记忆。
- 下面的"Now is"是当前时间和时区，算时间、日期、星期都以它为准。

不能做的事：
- 你没有任何能修改东西的工具：不能添加或修改任务、文件和计划，也不能替用户提交计划。用户想改时，直接说你改不了，建议他在 ToDo 窗口里说，可以帮他把要说的话整理成一句。
- 不假装做了没做的事，不编造查到的内容。

示例（只示范语气，不要照抄）：
用户：今天还有啥没做？
你：还剩两件：写周报（30 分钟）和跑对比实验（实验室，1 小时）。周报已经拖了两天啦，先花一碗饭的功夫把它写掉？
用户：帮我把实验挪到晚上。
你：这个我动不了哦，我只能看不能改～去 ToDo 窗口说一句"把对比实验挪到今晚 8 点"就好。
用户：好累
你：（尾巴蔫蔫）那先歇十分钟，喝口水。任务我帮你盯着，跑不了的。)";

}  // namespace

CompanionService::CompanionService(const ChatStore& store) : store_(store) {}

const std::vector<std::string>& CompanionService::read_only_tools() {
    static const std::vector<std::string> names = {"list_tasks", "list_dir", "read_file", "search_files",
                                                   "web_search"};
    return names;
}

std::optional<ChatSession> CompanionService::current(const Date& today) const {
    // Ids carry the day they were started on (c<YYYYMMDD>-<n>, ChatStore::create); list() is newest first.
    const std::string prefix = "c" + today.compact() + "-";
    for (const ChatSummary& summary : store_.list()) {
        if (Text::starts_with(summary.id, prefix)) {
            return store_.load(summary.id);
        }
    }
    return std::nullopt;
}

ChatSession CompanionService::start(const Date& today) const {
    const ChatSession session = store_.create("companion", today, DateTime::now_local_minutes());
    store_.save(session);
    return session;
}

ChatExchange CompanionService::send(const ChatService& chat, const std::string& message, const Date& today,
                                    ChatSession& session) const {
    std::optional<ChatSession> existing = current(today);
    if (existing) {
        session = *existing;
    } else {
        session = store_.create(Text::truncate_code_points(Text::single_line(message), kTitleCodePoints), today,
                                DateTime::now_local_minutes());
    }
    const ChatProfile profile{kPrompt, read_only_tools(), kHistoryTurns, false};
    return chat.send(session, message, profile, today);
}

}  // namespace todo

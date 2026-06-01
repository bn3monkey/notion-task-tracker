#pragma once
#include <string>

// Shared context for all commands.
struct Context {
    bool json = false;  // --json: emit machine-readable output
};

// ---- init ----
struct InitArgs {
    std::string token;        // optional; if empty, keep existing / rely on env
    std::string database_id;  // optional
};
int cmd_init(const Context& ctx, const InitArgs& args);

// ---- create (create a new DB with the full schema under a parent page) ----
struct CreateArgs {
    std::string parent_page_id;
    std::string title;  // DB title; default applied if empty
};
int cmd_create(const Context& ctx, const CreateArgs& args);

// ---- setup (add any missing fields to the already-configured DB) ----
int cmd_setup(const Context& ctx);

// ---- check (validate configured DB schema) ----
int cmd_check(const Context& ctx);

// ---- start (create a tracking page, return new 6-char ID) ----
struct StartArgs {
    std::string title;
    std::string deadline;  // YYYY-MM-DD, optional
    std::string priority;  // optional; default applied
    std::string status;    // optional; default applied
};
int cmd_start(const Context& ctx, const StartArgs& args);

// ---- resume (fetch an existing page by ID for context) ----
int cmd_resume(const Context& ctx, const std::string& id);

// ---- finish (append work summary, set end date / status) ----
struct FinishArgs {
    std::string id;
    std::string summary;     // inline summary; ignored if from_stdin
    bool from_stdin = false; // read summary from stdin
    std::string status;      // optional; default = 작업 완료
    bool keep_open = false;  // if set, keep current status
    std::string priority;    // optional; update 우선 순위
    std::string deadline;    // optional; update 마감일 (YYYY-MM-DD)
};
int cmd_finish(const Context& ctx, const FinishArgs& args);

// ---- search (find pages by title text, return IDs) ----
struct SearchArgs {
    std::string query;
    std::string status;  // optional filter
    int limit = 10;
};
int cmd_search(const Context& ctx, const SearchArgs& args);

// ---- guide (print LLM/user usage guide; single source of truth) ----
int cmd_guide(const Context& ctx);

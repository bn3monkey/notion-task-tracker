#include <CLI/CLI.hpp>

#include "commands/commands.hpp"
#include "notion_client.hpp"

int main(int argc, char** argv) {
    CLI::App app{"ntt — Notion 업무 트래커"};
    app.require_subcommand(1);
    app.fallthrough();  // allow --json to appear after the subcommand

    Context ctx;
    app.add_flag("--json", ctx.json, "구조화(JSON) 출력");

    int rc = 0;

    // ---- init ----
    InitArgs init_args;
    auto* init = app.add_subcommand("init", "연결 설정 저장 (token / database id)");
    init->add_option("--token", init_args.token, "Notion integration token");
    init->add_option("--db", init_args.database_id, "task-tracker database id");
    init->callback([&] { rc = cmd_init(ctx, init_args); });

    // ---- create (new DB) ----
    CreateArgs create_args;
    auto* create = app.add_subcommand("create", "새 DB를 올바른 스키마로 생성 (부모 페이지 아래)");
    create->add_option("--parent", create_args.parent_page_id, "부모 페이지 id")->required();
    create->add_option("--title", create_args.title, "DB 제목 (기본: 업무 트래커)");
    create->callback([&] { rc = cmd_create(ctx, create_args); });

    // ---- setup (add missing fields to the configured existing DB) ----
    auto* setup = app.add_subcommand("setup", "기존 DB에 누락된 필드 자동 추가");
    setup->callback([&] { rc = cmd_setup(ctx); });

    // ---- check ----
    auto* check = app.add_subcommand("check", "설정된 DB 스키마 검증 (불일치 시 exit 2)");
    check->callback([&] { rc = cmd_check(ctx); });

    // ---- start ----
    StartArgs start_args;
    auto* start = app.add_subcommand("start", "트래킹 시작 (새 페이지 생성, 6자리 ID 발급)");
    start->add_option("--title", start_args.title, "업무명")->required();
    start->add_option("--deadline", start_args.deadline, "마감일 YYYY-MM-DD");
    start->add_option("--priority", start_args.priority,
                      "우선순위 키 (ui/un/ai/an/si/sn) 또는 정식 명칭");
    start->add_option("--status", start_args.status, "초기 진행 상황 (예정/중/완료/보류/검토/장기/폐기)");
    start->callback([&] { rc = cmd_start(ctx, start_args); });

    // ---- resume ----
    std::string resume_id;
    auto* resume = app.add_subcommand("resume", "트래킹 이어하기 (기존 페이지 조회)");
    resume->add_option("id", resume_id, "식별용 ID")->required();
    resume->callback([&] { rc = cmd_resume(ctx, resume_id); });

    // ---- finish ----
    FinishArgs finish_args;
    auto* finish = app.add_subcommand("finish", "트래킹 종료 (요약 기록, 종료일/상태 갱신)");
    finish->add_option("id", finish_args.id, "식별용 ID")->required();
    finish->add_option("--summary", finish_args.summary, "업무 요약 (짧을 때)");
    finish->add_flag("--stdin", finish_args.from_stdin, "요약을 stdin에서 읽기 (권장)");
    finish->add_option("--status", finish_args.status,
                       "종료 상태 (완료/보류/검토/장기/폐기, 기본: 작업 완료)");
    finish->add_flag("--keep-open", finish_args.keep_open, "상태를 바꾸지 않음 (중간 저장)");
    finish->add_option("--priority", finish_args.priority,
                       "우선순위 키 (ui/un/ai/an/si/sn) 또는 정식 명칭");
    finish->add_option("--deadline", finish_args.deadline, "마감일 갱신 YYYY-MM-DD");
    finish->callback([&] { rc = cmd_finish(ctx, finish_args); });

    // ---- search ----
    SearchArgs search_args;
    auto* search = app.add_subcommand("search", "업무 검색 (제목 키워드 → ID 목록)");
    search->add_option("query", search_args.query, "검색 키워드");
    search->add_option("--status", search_args.status, "진행 상황 필터");
    search->add_option("--limit", search_args.limit, "최대 결과 수 (기본 10)");
    search->callback([&] { rc = cmd_search(ctx, search_args); });

    // ---- guide ----
    auto* guide = app.add_subcommand("guide", "에이전트/사용자용 사용법 출력");
    guide->callback([&] { rc = cmd_guide(ctx); });

    NotionClient::global_init();
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        NotionClient::global_cleanup();
        return app.exit(e);
    }
    NotionClient::global_cleanup();
    return rc;
}

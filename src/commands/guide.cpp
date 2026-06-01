#include <iostream>

#include "command_support.hpp"

namespace {

// Single source of truth for ntt usage. The install scripts capture this output
// to generate the Claude Code skill / append to a project's CLAUDE.md.
const char* kGuide = R"GUIDE(# ntt — Notion 업무 트래커 사용법 (에이전트용)

`ntt`는 내가 한 업무를 Notion task-tracker DB에 기록하는 CLI다.
모든 명령은 `--json` 플래그로 구조화 출력을 받을 수 있다 (에이전트는 항상 `--json` 사용 권장).
긴 텍스트(업무 요약)는 인자 대신 stdin으로 전달한다.

## 언제 무엇을 호출하나

- 새 업무를 시작할 때        → `ntt start --title "<업무명>" --json`  → 반환된 `id`(6자리)를 기억
- 기존 업무를 이어서 할 때    → `ntt resume <id> --json`  (기존 진행 상황·기록을 읽어 컨텍스트로 사용)
- 업무를 마치거나 중간 정리   → 요약을 stdin으로 `ntt finish <id> --stdin --json`
- 어떤 업무였는지 ID를 잊음   → `ntt search "<키워드>" --json`  → 후보들의 `id` 반환

## 명령 레퍼런스

- `ntt init --token <t> [--db <database_id>]`   연결 설정 저장
- `ntt setup --parent <page_id> [--title "..."]` 올바른 스키마로 DB 생성(부모 페이지 아래)
- `ntt check`                                    설정된 DB 스키마 검증 (불일치 시 exit 2)
- `ntt start --title "..." [--deadline YYYY-MM-DD] [--priority 높음|보통|낮음] [--status ...]`
- `ntt resume <id>`                              기존 페이지 속성·본문 출력
- `ntt finish <id> [--stdin | --summary "..."] [--status ...] [--keep-open]`
- `ntt search "<query>" [--status ...] [--limit N]`
- `ntt guide`                                    이 안내 출력

## 전형적 흐름

```bash
ID=$(ntt start --title "결제 모듈 리팩터링" --priority 높음 --json | jq -r .id)
# ... 작업 수행 ...
echo "PG 연동 추상화 인터페이스 분리, 단위테스트 12개 추가" | ntt finish "$ID" --stdin --json
```

## 스키마 (DB 속성)

| 속성 | 타입 | 값 |
|------|------|----|
| 제목 | Title | 업무명 |
| 식별용 ID | Rich text | 6자리 [A-Za-z0-9] |
| 수행기간 | Date | 시작~종료 |
| 마감일 | Date | |
| 진행 상황 | Select | 시작 전 / 진행중 / 완료 |
| 우선 순위 | Select | 높음 / 보통 / 낮음 |

## 주의

- `ntt finish`는 기본적으로 상태를 '완료'로 바꾼다. 중간 저장이면 `--keep-open`을 쓴다.
- 토큰은 NTT_NOTION_TOKEN 환경변수 또는 `ntt init`로 저장된 값을 사용한다.
)GUIDE";

}  // namespace

int cmd_guide(const Context& ctx) {
    if (ctx.json) {
        cmd::ok_json({{"ok", true}, {"guide", kGuide}});
    } else {
        std::cout << kGuide << "\n";
    }
    return 0;
}

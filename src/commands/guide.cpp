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

- `ntt init --token <t> [--db <database_id>]`   연결 설정 저장 (기존 DB는 --db로 지정)
- `ntt create --parent <page_id> [--title "..."]` 새 DB를 올바른 스키마로 생성
- `ntt setup`                                    기존(연결된) DB에 누락된 필드만 자동 추가
- `ntt check`                                    설정된 DB 스키마 검증 (불일치 시 exit 2)
- `ntt start --title "..." [--deadline YYYY-MM-DD] [--priority <키>] [--status <키>]`
- `ntt resume <id>`                              기존 페이지 속성·본문 출력
- `ntt finish <id> [--stdin|--summary "..."] [--status <키>|--keep-open] [--priority <키>] [--deadline YYYY-MM-DD]`
- `ntt search "<query>" [--status ...] [--limit N]`
- `ntt guide`                                    이 안내 출력

## 입력 키워드 (start / finish 공통)

- **우선 순위** `--priority` (6값만 허용, 짧은 키 권장):
  - `ui`=Urgent & Important, `un`=Urgent & Not Important
  - `ai`=ASAP & Important,   `an`=ASAP & Not Important
  - `si`=Someday & Important, `sn`=Someday & Not Important
  - 정식 명칭도 그대로 허용. 그 외 값은 거부된다.
- **진행 상황** `--status`: `예정`/`중`/`완료`/`보류`/`검토`/`장기`/`폐기` (또는 정식 명칭)
  - `start` 기본 = 작업 중, `finish` 기본 = 작업 완료
  - `finish --keep-open`은 상태를 그대로 둔다(중간 저장).
  - `finish --status 보류|검토|장기|폐기 …`로 전환 가능.
- **태그 재활용**: 진행 상황·우선 순위를 쓸 때, 그 키워드를 **포함하는 기존 select 태그**가
  있으면(예: "🟢 작업 완료") 새 태그를 만들지 않고 그 태그를 그대로 사용한다.
- `finish --deadline YYYY-MM-DD`로 마감일을 갱신할 수 있다.
- `finish` 시 수행기간은 (start에서 기록된 시작일) ~ (finish 호출일)로 설정된다.

## 기존 DB로 시작하는 흐름

```bash
ntt init --token <secret_...> --db <기존_database_id>
ntt setup --json     # 없는 필드(식별용 ID/수행기간/마감일/진행 상황/우선 순위)만 추가
ntt check --json     # 스키마 검증
```

## 전형적 트래킹 흐름

```bash
ID=$(ntt start --title "결제 모듈 리팩터링" --priority "ASAP & Important" --json | jq -r .id)
# ... 작업 수행 ...
echo "PG 연동 추상화 인터페이스 분리, 단위테스트 12개 추가" | ntt finish "$ID" --stdin --json
```

## 스키마 (속성 매칭 규칙)

| 논리 필드 | Notion 타입 | 인식되는 컬럼 이름 |
|-----------|-------------|--------------------|
| 제목      | Title       | **페이지 이름(title 타입) 자체** — 별도 컬럼 불필요, 이름 무관 |
| 식별용 ID | Rich text   | `식별용 ID` |
| 수행기간  | Date        | `수행기간` 또는 `수행 기간` |
| 마감일    | Date        | `마감일` |
| 진행 상황 | Select      | `진행 상황` |
| 우선 순위 | Select      | `우선 순위` 또는 `우선순위` |

- `ntt`는 기존 DB의 컬럼 이름을 위 후보군으로 자동 인식한다. `setup`/`create`로 만들 땐 첫 번째(정식) 이름을 쓴다.
- 기존 DB의 select 옵션은 건드리지 않는다. 새로 만들 때만 아래 기본 옵션을 넣는다.

### 필드 생성 시 기본 옵션

- **진행 상황**: 작업 예정 / 작업 중 / 작업 완료 / 작업 보류 / 검토 대기 / 장기 / 폐기
- **우선 순위**: Urgent & Important / Urgent & Not Important / ASAP & Important / ASAP & Not Important / Someday & Important / Someday & Not Important

## 기타

- 우선 순위는 `--priority`를 줄 때만 설정된다.
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

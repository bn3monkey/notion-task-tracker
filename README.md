# ntt — Notion 업무 트래커

내가 한 업무를 Notion task-tracker DB에 자동으로 기록하는 C++ 단일 바이너리 CLI.
Claude(또는 사용자)가 간단한 명령으로 업무 트래킹을 시작/이어하기/종료할 수 있다.

설계 상세는 [requirement.md](requirement.md), 개발/빌드는 [CLAUDE.md](CLAUDE.md) 참고.

## 인증 (1회만)

`ntt`는 **Notion Integration Token** 하나로 동작한다. Claude는 토큰을 몰라도 되고,
Notion MCP 커넥터도 필요 없다 (이중 인증 방지).

1. <https://www.notion.so/my-integrations> 에서 **Internal Integration** 생성 → 토큰 복사
2. 트래커를 둘 **부모 페이지**를 열고 `•••` → **Connections** → 방금 만든 integration 연결
3. `ntt init --token <secret_...>`

## 빌드

```bash
./script/generate-msvc-build-script.sh   # 머신별 build.sh 생성 (VS/cmake 자동 탐색)
./script/build.sh                        # 빌드 → out/build/x64-Debug/ntt.exe
```

의존성(curl, nlohmann/json, CLI11)은 CMake **FetchContent**(tarball)로 자동 취득한다. vcpkg 불필요.

## 설치

레포를 클론하지 않고 한 줄로 설치할 수 있다 (GitHub Release의 prebuilt 바이너리를 받는다).
현재 prebuilt 제공 대상은 **Windows x64 / Linux x64**. macOS·arm64는 소스에서 빌드.

```powershell
# Windows (PowerShell): 전역 설치 + PATH 등록 + Claude Code Skill 생성
irm https://github.com/bn3monkey/notion-task-tracker/releases/latest/download/install.ps1 | iex
```

```bash
# Linux / git-bash: 전역 설치(~/.local/bin) + Claude Code Skill
curl -fsSL https://github.com/bn3monkey/notion-task-tracker/releases/latest/download/install.sh | bash
```

> 특정 버전을 받으려면 `NTT_VERSION=vX.Y.Z` 환경변수를 지정한다.

레포를 직접 받아 쓰는 경우(로컬 빌드본 우선 사용):

```powershell
# 전역 설치 (PATH 등록 + Claude Code Skill 생성)
pwsh scripts/install.ps1 -Global

# 특정 프로젝트에 tools/로 설치 + 그 프로젝트 CLAUDE.md에 사용법 append
pwsh scripts/install.ps1 -Dir D:\repo\my-project
```

```bash
# Linux/macOS/git-bash
./scripts/install.sh --global
./scripts/install.sh --dir /path/to/project
```

## 사용

### 새 DB를 만들 때

```bash
ntt init --token <secret_...>          # 연결 설정
ntt create --parent <page_id>          # 새 DB를 올바른 스키마로 생성 (+ db_id 저장)
ntt check                              # 스키마 검증
```

### 기존 DB를 쓸 때

```bash
ntt init --token <secret_...> --db <기존_database_id>
ntt setup                              # 없는 필드만 자동 추가 (기존 컬럼/옵션은 보존)
ntt check
```

### 트래킹

```bash
ID=$(ntt start --title "결제 모듈 리팩터링" --priority "ASAP & Important" --json | jq -r .id)
# ... 작업 ...
echo "PG 추상화 분리, 테스트 12개 추가" | ntt finish "$ID" --stdin --json

ntt resume <ID>                        # 기존 업무 이어하기
ntt search "결제" --json               # 업무 검색 → ID 목록
ntt guide                              # 에이전트/사용자용 사용법
```

- `create` = 새 DB 생성, `setup` = 기존 DB에 누락 필드 추가. 둘은 다른 명령이다.
- 에이전트는 `--json`으로 구조화 출력을 받는다.
- 긴 요약은 인자 대신 stdin으로 전달한다.
- 토큰은 `NTT_NOTION_TOKEN` 환경변수로도 줄 수 있다 (config보다 우선).

## DB 스키마 (속성 매칭)

`ntt`는 기존 DB의 컬럼 이름을 아래 후보군으로 **자동 인식**한다. 없으면 `ntt setup`이 정식 이름으로 추가한다.

| 논리 필드 | 타입 | 인식되는 컬럼 이름 |
|-----------|------|--------------------|
| 제목 | Title | **페이지 이름(title 타입) 자체** — 별도 컬럼 불필요, 이름 무관 |
| 식별용 ID | Rich text | `식별용 ID` (6자리 [A-Za-z0-9]) |
| 수행기간 | Date | `수행기간` 또는 `수행 기간` |
| 마감일 | Date | `마감일` |
| 진행 상황 | Select | `진행 상황` |
| 우선 순위 | Select | `우선 순위` 또는 `우선순위` |

### 필드 생성 시 기본 옵션 (`create` / `setup`)

기존 select의 옵션은 보존하며, **새로 만들 때만** 아래 옵션을 넣는다.

- **진행 상황**: 작업 예정 / 작업 중 / 작업 완료 / 작업 보류 / 검토 대기 / 장기 / 폐기
- **우선 순위**: Urgent & Important / Urgent & Not Important / ASAP & Important / ASAP & Not Important / Someday & Important / Someday & Not Important

### 입력 키워드 (`start` / `finish` 공통)

| 옵션 | 입력 | 비고 |
|------|------|------|
| `--priority` | `ui`/`un`/`ai`/`an`/`si`/`sn` 또는 정식 명칭 | 6값만 허용, 그 외 거부 |
| `--status` | `예정`/`중`/`완료`/`보류`/`검토`/`장기`/`폐기` 또는 정식 명칭 | |

- 짧은 키 규칙: 긴급도(`u`rgent/`a`sap/`s`omeday) + 중요도(`i`mportant/`n`ot).
- **상태 기본값**: `start` → 작업 중, `finish` → 작업 완료. `finish --keep-open`이면 유지.
- `finish`로 `--status`(전환)·`--priority`·`--deadline`을 함께 갱신할 수 있다.
- **태그 재활용**: 진행 상황·우선 순위를 쓸 때 그 키워드를 **포함하는 기존 태그**가 있으면
  (예: `🟢 작업 완료`) 새 태그를 만들지 않고 그 태그를 사용한다.
- `finish` 시 수행기간 = (start 시작일) ~ (finish 호출일).

## 종료 코드

| 코드 | 의미 |
|------|------|
| 0 | 성공 |
| 1 | 일반 오류 (토큰/DB 미설정, API 오류 등) |
| 2 | `check` 스키마 불일치 |
| 3 | `resume`/`finish` 대상 ID 없음 |

# Notion 업무 추적기 (ntt)

## 목적

내가 한 업무를 Notion 업무 트래커 데이터베이스(task-tracker DB)에 자동으로 기록한다.
Claude가 간단한 CLI 명령(`ntt ...`)으로 트래킹을 시작/이어하기/종료할 수 있게 한다.

## 아키텍처

```
Claude ──Bash──▶ ntt (C++ 단일 바이너리) ──HTTPS──▶ api.notion.com/v1
                      │
                      └─ config.json (Integration Token, database_id)
```

- 인증은 **1회**: Notion Integration Token을 `ntt init` 시 저장. 이후 Claude는 토큰을 모름.
- Claude는 "무슨 일을 했는지" 판단·요약만 하고, 실제 Notion 호출은 전부 바이너리가 수행.
- Notion MCP 커넥터는 이 워크플로에 불필요 (이중 인증 방지).

## 기술 스택

| 영역 | 선택 |
|------|------|
| 언어 | C++17 |
| 빌드 | CMake (+ `d:\claude_config\msvc_cmake.sh`) |
| 의존성 | vcpkg: curl, nlohmann-json, cli11 |
| HTTP | libcurl (Windows: Schannel 백엔드 → OpenSSL 불필요) |
| JSON | nlohmann/json |
| CLI | CLI11 |

## DB 스키마 (task-tracker)

| 속성 이름 | Notion 타입 | 비고 |
|----------|------------|------|
| 제목 | Title | Page 제목 |
| 식별용 ID | Rich text | `[A-Za-z0-9]{6}` |
| 수행기간 | Date | start~end range |
| 마감일 | Date | |
| 진행 상황 | Select | 시작 전 / 진행중 / 완료 |
| 우선 순위 | Select | 높음 / 보통 / 낮음 |

## Use Case → CLI

1. **설치**: 설치 스크립트로 단일 바이너리 설치
   1. 전역 디렉토리에 설치 + 환경 변수 등록 → Claude Code **Skill** 추가
   2. 사용자 지정 디렉토리(tools)에 설치 → 해당 프로젝트 **CLAUDE.md**에 사용법 append
2. **연결**: `ntt init --token <t> [--db <database_id>]`
3. **스키마 검증**: `ntt check` (불일치 시 exit ≠ 0)
4. **DB 생성**: `ntt setup --parent <page_id> [--title "..."]` (스키마 맞춰 생성 + db_id 저장)
5. **트래킹 시작**: `ntt start --title "..." [--deadline YYYY-MM-DD] [--priority ...]` → 6자리 ID 발급, Page 생성, **ID를 stdout 출력**
6. **트래킹 이어하기**: `ntt resume <ID>` → 기존 Page 속성·본문 출력(Claude 컨텍스트용)
7. **트래킹 종료**: `... | ntt finish <ID> --stdin [--status 완료]` → 본문에 요약 append, 수행기간 end=오늘, 상태 갱신
8. **검색**: `ntt search "<query>" [--status ...]` → 매칭 Page들의 ID 목록 반환

### 공통

- 전역 플래그 `--json`: Claude용 구조화 출력 (`{"id":"Ab3Xy9","page_id":"...","url":"..."}`)
- 긴 요약은 stdin으로 전달 (인자 길이 한계 회피)
- `ntt guide`: LLM/사용자용 사용법 출력 (Skill·CLAUDE.md 생성의 단일 소스)

## 설정 파일

- 경로: `%APPDATA%\ntt\config.json` (Windows) / `$XDG_CONFIG_HOME/ntt/config.json` (unix)
- 토큰 우선순위: 환경변수 `NTT_NOTION_TOKEN` > config.json

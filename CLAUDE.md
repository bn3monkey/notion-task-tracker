# notion-task-tracker (ntt)

Notion 업무 트래커 DB에 업무를 기록하는 C++ 단일 바이너리 CLI. 설계는 [requirement.md](requirement.md) 참고.

## 빌드 (MSVC + CMake)

이 프로젝트는 vcpkg를 쓰지 않고 **CMake FetchContent**로 의존성(curl, nlohmann/json, CLI11)을 가져온다.
빌드는 **빌드 스크립트 생성기**를 먼저 돌려 머신별 `script/build.sh`를 만든 뒤 실행한다.

```bash
# 1) 빌드 스크립트 생성 (VsDevCmd / cmake / 빌드 디렉토리 자동 탐색 → script/build.sh)
./script/generate-msvc-build-script.sh

# 2) 빌드
./script/build.sh            # 증분 빌드 (캐시 없으면 자동 재구성)
./script/build.sh --rebuild  # 캐시 초기화 후 재구성 + 빌드 (소스 추가 시)
./script/build.sh --clean    # 빌드 산출물만 삭제
```

- 기본 제너레이터/빌드타입: **Ninja / Debug**, 기본 빌드 디렉토리: `out/build/x64-Debug`
- 산출물: `<build-dir>/ntt.exe`
- `script/build.sh`는 머신마다 절대 경로가 달라 `.gitignore` 처리됨. 환경이 바뀌면 생성기를 다시 실행.

## 구조

```
src/
  main.cpp              CLI11 와이어링, --json 전역 플래그
  config.{hpp,cpp}      config.json 로드/세이브, NTT_NOTION_TOKEN
  notion_client.{hpp,cpp}  api.notion.com/v1 HTTP 래퍼 (libcurl)
  schema.hpp            DB 속성 이름·타입 정의
  id_gen.{hpp,cpp}      6자리 [A-Za-z0-9] ID 생성
  util.{hpp,cpp}        날짜/문자열/stdin 헬퍼
  commands/             init, setup, check, start, resume, finish, search, guide
scripts/                install.ps1 / install.sh
```

## 규약

- Claude가 호출하므로 모든 명령은 `--json`으로 구조화 출력 가능해야 한다.
- 실패 시 stderr + exit code ≠ 0. `check`는 스키마 불일치 시 비정상 종료.
- 긴 텍스트(요약 등)는 인자 대신 stdin으로 받는다.

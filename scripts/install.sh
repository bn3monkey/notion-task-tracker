#!/usr/bin/env bash
# ntt 설치 스크립트 (Linux/macOS, 또는 git-bash).
#
# 사용법:
#   curl -fsSL .../install.sh | bash            전역 설치 (기본값: --global)
#   ./scripts/install.sh --global              전역 설치(~/.local/bin) + Claude Code Skill
#   ./scripts/install.sh --dir <project_dir>   프로젝트 tools/에 설치 + CLAUDE.md append
#   옵션: --binary <path>   빌드된 ntt 바이너리 경로 직접 지정
#         NTT_VERSION=vX.Y.Z 환경변수로 받을 릴리스 버전 지정 (기본: latest)
#
# 로컬 빌드본(out/build/)이 있으면 그걸 쓰고, 없으면 GitHub Release에서 받는다.
set -euo pipefail

REPO="bn3monkey/notion-task-tracker"

MODE=""
DIR=""
BINARY=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --global)  MODE="global"; shift ;;
        --dir)     MODE="dir"; DIR="$2"; shift 2 ;;
        --binary)  BINARY="$2"; shift 2 ;;
        -h|--help) sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "[install] 알 수 없는 인자: $1" >&2; exit 2 ;;
    esac
done
# curl | bash 로 인자 없이 실행되면 전역 설치를 기본으로 한다.
[[ -z "$MODE" ]] && MODE="global"

# 레포 안에서 실행됐을 때만 REPO_ROOT 를 잡는다 (curl | bash 시엔 비어 있음).
REPO_ROOT=""
if [[ -n "${BASH_SOURCE[0]:-}" && -f "${BASH_SOURCE[0]}" ]]; then
    REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fi

# ----- 릴리스에서 바이너리 다운로드 ------------------------------------------
download_binary() {
    local os asset url tmp
    case "$(uname -s)" in
        Linux)  asset="ntt-linux-x64" ;;
        Darwin) echo "[install] macOS prebuilt 바이너리는 아직 없습니다. 소스에서 빌드하세요." >&2; exit 1 ;;
        *)      echo "[install] 지원하지 않는 OS: $(uname -s). 소스에서 빌드하세요." >&2; exit 1 ;;
    esac
    case "$(uname -m)" in
        x86_64|amd64) ;;
        *) echo "[install] 지원하지 않는 아키텍처: $(uname -m) (x86_64만 제공). 소스에서 빌드하세요." >&2; exit 1 ;;
    esac

    local version="${NTT_VERSION:-latest}"
    if [[ "$version" == "latest" ]]; then
        url="https://github.com/$REPO/releases/latest/download/$asset"
    else
        url="https://github.com/$REPO/releases/download/$version/$asset"
    fi

    tmp="$(mktemp -d)/ntt"
    echo "[install] 릴리스에서 다운로드: $url" >&2
    if command -v curl >/dev/null 2>&1; then
        curl -fSL "$url" -o "$tmp"
    elif command -v wget >/dev/null 2>&1; then
        wget -qO "$tmp" "$url"
    else
        echo "[install] curl 또는 wget 이 필요합니다." >&2; exit 1
    fi
    chmod +x "$tmp"
    echo "$tmp"
}

# ----- 바이너리 위치 결정 ----------------------------------------------------
if [[ -z "$BINARY" && -n "$REPO_ROOT" ]]; then
    BINARY="$(find "$REPO_ROOT/out/build" -name 'ntt' -o -name 'ntt.exe' 2>/dev/null \
              | xargs -r ls -t 2>/dev/null | head -n 1 || true)"
fi
if [[ -z "$BINARY" || ! -f "$BINARY" ]]; then
    BINARY="$(download_binary)"
fi
echo "[install] 바이너리: $BINARY"

get_guide() { "$BINARY" guide; }

install_global() {
    local target="$HOME/.local/bin"
    mkdir -p "$target"
    cp "$BINARY" "$target/ntt"
    chmod +x "$target/ntt"
    echo "[install] 설치: $target/ntt"
    case ":$PATH:" in
        *":$target:"*) echo "[install] PATH에 이미 존재" ;;
        *) echo "[install] 주의: $target 가 PATH에 없습니다. 셸 rc에 추가하세요." ;;
    esac

    local skill_dir="$HOME/.claude/skills/ntt"
    mkdir -p "$skill_dir"
    {
        printf -- '---\n'
        printf 'name: ntt\n'
        printf 'description: Notion 업무 트래커. 업무 시작/이어하기/종료를 ntt CLI로 Notion task-tracker DB에 기록한다. 사용자가 한 업무를 노션에 정리/저장하려 할 때 사용.\n'
        printf -- '---\n\n'
        get_guide
    } > "$skill_dir/SKILL.md"
    echo "[install] Claude Code Skill 생성: $skill_dir/SKILL.md"
}

install_dir() {
    local project_dir; project_dir="$(cd "$DIR" && pwd)"
    local tools_dir="$project_dir/tools"
    mkdir -p "$tools_dir"
    cp "$BINARY" "$tools_dir/$(basename "$BINARY")"
    echo "[install] 설치: $tools_dir/$(basename "$BINARY")"

    # CLAUDE.md 탐색 (위로 올라가며), 없으면 project_dir에 생성
    local claude_md="" dir="$project_dir"
    while [[ -n "$dir" && "$dir" != "/" ]]; do
        [[ -f "$dir/CLAUDE.md" ]] && { claude_md="$dir/CLAUDE.md"; break; }
        dir="$(dirname "$dir")"
    done
    [[ -z "$claude_md" ]] && claude_md="$project_dir/CLAUDE.md"

    local begin='<!-- ntt:begin -->' end='<!-- ntt:end -->'
    local section
    section="$(printf '%s\n## ntt — Notion 업무 트래커 (tools/ntt)\n\n%s\n%s\n' "$begin" "$(get_guide)" "$end")"

    if [[ -f "$claude_md" ]] && grep -qF "$begin" "$claude_md"; then
        # 기존 섹션 교체 (awk로 begin~end 구간 치환)
        local tmp; tmp="$(mktemp)"
        awk -v b="$begin" -v e="$end" -v repl="$section" '
            $0 ~ b {print repl; skip=1; next}
            $0 ~ e {skip=0; next}
            !skip {print}
        ' "$claude_md" > "$tmp"
        mv "$tmp" "$claude_md"
        echo "[install] CLAUDE.md ntt 섹션 갱신: $claude_md"
    else
        printf '\n\n%s\n' "$section" >> "$claude_md"
        echo "[install] CLAUDE.md에 ntt 섹션 추가: $claude_md"
    fi
}

if [[ "$MODE" == "global" ]]; then install_global; else install_dir; fi
echo "[install] 완료. 다음 단계:"
echo "  기존 DB:  ntt init --token <TOKEN> --db <database_id> && ntt setup && ntt check"
echo "  새  DB:   ntt init --token <TOKEN> && ntt create --parent <page_id>"

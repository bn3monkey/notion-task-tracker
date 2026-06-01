#!/usr/bin/env bash
# ntt 설치 스크립트 (Linux/macOS, 또는 git-bash).
#
# 사용법:
#   ./scripts/install.sh --global              전역 설치(~/.local/bin) + Claude Code Skill
#   ./scripts/install.sh --dir <project_dir>   프로젝트 tools/에 설치 + CLAUDE.md append
#   옵션: --binary <path>   빌드된 ntt 바이너리 경로 직접 지정
set -euo pipefail

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
[[ -z "$MODE" ]] && { echo "[install] --global 또는 --dir <path>가 필요합니다." >&2; exit 2; }

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# ----- 바이너리 위치 결정 ----------------------------------------------------
if [[ -z "$BINARY" ]]; then
    BINARY="$(find "$REPO_ROOT/out/build" -name 'ntt' -o -name 'ntt.exe' 2>/dev/null \
              | xargs -r ls -t 2>/dev/null | head -n 1 || true)"
fi
[[ -n "$BINARY" && -f "$BINARY" ]] || {
    echo "[install] 빌드된 ntt를 찾지 못했습니다. 먼저 빌드하세요." >&2; exit 1; }
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
echo "[install] 완료. 다음: ntt init --token <NOTION_TOKEN> && ntt setup --parent <page_id>"

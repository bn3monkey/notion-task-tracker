<#
.SYNOPSIS
  ntt 바이너리를 설치하고, 사용법을 Claude/사용자에게 자동으로 익히게 한다.

.DESCRIPTION
  두 가지 모드:
    -Global        전역 디렉토리(%LOCALAPPDATA%\Programs\ntt)에 설치 + 사용자 PATH 등록
                   + Claude Code 전역 Skill(~/.claude/skills/ntt/SKILL.md) 생성.
    -Dir <path>    지정한 프로젝트 디렉토리의 tools/에 설치 + 그 프로젝트의 CLAUDE.md에
                   ntt 사용법을 append.

  로컬 빌드본(out\build\)이 있으면 그걸 쓰고, 없으면 GitHub Release에서 받는다.
  -BinaryPath로 직접 지정 가능. 받을 릴리스 버전은 $env:NTT_VERSION (기본: latest).

.EXAMPLE
  irm https://github.com/bn3monkey/notion-task-tracker/releases/latest/download/install.ps1 | iex
  pwsh scripts/install.ps1 -Global
  pwsh scripts/install.ps1 -Dir D:\repo\my-project
#>
[CmdletBinding(DefaultParameterSetName = 'Global')]
param(
    [Parameter(ParameterSetName = 'Global')]
    [switch]$Global,

    [Parameter(ParameterSetName = 'Dir', Mandatory = $true)]
    [string]$Dir,

    [string]$BinaryPath
)

$ErrorActionPreference = 'Stop'
$Repo = 'bn3monkey/notion-task-tracker'
# 레포 안에서 실행됐을 때만 repoRoot 를 잡는다 (irm | iex 시엔 비어 있음).
$repoRoot = if ($PSScriptRoot) { Split-Path -Parent $PSScriptRoot } else { $null }

# ----- 1. 바이너리 위치 결정 -------------------------------------------------
function Get-ReleaseBinary {
    $version = if ($env:NTT_VERSION) { $env:NTT_VERSION } else { 'latest' }
    $asset = 'ntt-windows-x64.exe'
    $url = if ($version -eq 'latest') {
        "https://github.com/$Repo/releases/latest/download/$asset"
    } else {
        "https://github.com/$Repo/releases/download/$version/$asset"
    }
    $tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("ntt-" + [guid]::NewGuid().ToString('N') + ".exe")
    Write-Host "[install] 릴리스에서 다운로드: $url"
    Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing
    return $tmp
}

function Resolve-Binary {
    if ($BinaryPath) {
        if (-not (Test-Path $BinaryPath)) { throw "지정한 바이너리가 없습니다: $BinaryPath" }
        return (Resolve-Path $BinaryPath).Path
    }
    if ($repoRoot) {
        $candidates = Get-ChildItem -Path (Join-Path $repoRoot 'out\build') -Recurse -Filter 'ntt.exe' -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending
        if ($candidates) { return $candidates[0].FullName }
    }
    return (Get-ReleaseBinary)
}

$binary = Resolve-Binary
Write-Host "[install] 바이너리: $binary"

# ----- 2. guide 텍스트 추출 (Skill / CLAUDE.md 생성의 단일 소스) -------------
function Get-Guide { & $binary guide }

# ----- 모드: Global ----------------------------------------------------------
function Install-Global {
    $target = Join-Path $env:LOCALAPPDATA 'Programs\ntt'
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    Copy-Item $binary (Join-Path $target 'ntt.exe') -Force
    Write-Host "[install] 설치: $target\ntt.exe"

    # PATH (사용자 범위) 등록
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    if ($userPath -notlike "*$target*") {
        [Environment]::SetEnvironmentVariable('Path', "$userPath;$target", 'User')
        Write-Host "[install] 사용자 PATH에 추가됨 (새 셸부터 적용)"
    } else {
        Write-Host "[install] PATH에 이미 존재"
    }

    # Claude Code 전역 Skill 생성
    $skillDir = Join-Path $env:USERPROFILE '.claude\skills\ntt'
    New-Item -ItemType Directory -Force -Path $skillDir | Out-Null
    $guide = Get-Guide
    $front = @"
---
name: ntt
description: Notion 업무 트래커. 업무 시작/이어하기/종료를 ntt CLI로 Notion task-tracker DB에 기록한다. 사용자가 한 업무를 노션에 정리/저장하려 할 때 사용.
---

"@
    $skillPath = Join-Path $skillDir 'SKILL.md'
    Set-Content -Path $skillPath -Value ($front + ($guide -join "`n")) -Encoding utf8
    Write-Host "[install] Claude Code Skill 생성: $skillPath"
}

# ----- 모드: Dir -------------------------------------------------------------
function Install-Dir {
    param([string]$ProjectDir)
    if (-not (Test-Path $ProjectDir)) { throw "디렉토리가 없습니다: $ProjectDir" }
    $ProjectDir = (Resolve-Path $ProjectDir).Path

    $toolsDir = Join-Path $ProjectDir 'tools'
    New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null
    Copy-Item $binary (Join-Path $toolsDir 'ntt.exe') -Force
    Write-Host "[install] 설치: $toolsDir\ntt.exe"

    # CLAUDE.md 탐색: ProjectDir에서 위로 올라가며 찾고, 없으면 ProjectDir에 생성
    $claudeMd = $null
    $dir = $ProjectDir
    while ($dir) {
        $cand = Join-Path $dir 'CLAUDE.md'
        if (Test-Path $cand) { $claudeMd = $cand; break }
        $parent = Split-Path -Parent $dir
        if ($parent -eq $dir) { break }
        $dir = $parent
    }
    if (-not $claudeMd) { $claudeMd = Join-Path $ProjectDir 'CLAUDE.md' }

    $guide = (Get-Guide) -join "`n"
    $begin = '<!-- ntt:begin -->'
    $end = '<!-- ntt:end -->'
    $section = "$begin`n## ntt — Notion 업무 트래커 (tools/ntt.exe)`n`n$guide`n$end"

    $existing = if (Test-Path $claudeMd) { Get-Content $claudeMd -Raw } else { '' }
    if ($existing -match [regex]::Escape($begin)) {
        # 기존 섹션 교체
        $pattern = "(?s)$([regex]::Escape($begin)).*?$([regex]::Escape($end))"
        $new = [regex]::Replace($existing, $pattern, [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $section })
        Set-Content -Path $claudeMd -Value $new -Encoding utf8
        Write-Host "[install] CLAUDE.md ntt 섹션 갱신: $claudeMd"
    } else {
        $sep = if ($existing.Trim().Length -gt 0) { "`n`n" } else { '' }
        Add-Content -Path $claudeMd -Value ($sep + $section) -Encoding utf8
        Write-Host "[install] CLAUDE.md에 ntt 섹션 추가: $claudeMd"
    }
}

# ----- 디스패치 --------------------------------------------------------------
if ($PSCmdlet.ParameterSetName -eq 'Dir') {
    Install-Dir -ProjectDir $Dir
} else {
    Install-Global
}

Write-Host "[install] 완료. 다음 단계:"
Write-Host "  기존 DB:  ntt init --token <TOKEN> --db <database_id> && ntt setup && ntt check"
Write-Host "  새  DB:   ntt init --token <TOKEN> && ntt create --parent <page_id>"

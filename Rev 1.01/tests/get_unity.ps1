# get_unity.ps1
#
# One-time setup: downloads the three Unity test framework source files from
# GitHub into tests/unity/ so the Makefile can compile them.
#
# Run once from the tests/ directory (or anywhere — $PSScriptRoot keeps paths correct):
#   powershell -ExecutionPolicy Bypass -File get_unity.ps1
#
# Alternatively, the same result from any terminal (Windows curl built-in):
#   mkdir unity
#   curl -LO --output-dir unity https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.c
#   curl -LO --output-dir unity https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.h
#   curl -LO --output-dir unity https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity_internals.h

$ErrorActionPreference = "Stop"

$base   = "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src"
$outDir = Join-Path $PSScriptRoot "unity"

New-Item -ItemType Directory -Force -Path $outDir | Out-Null

foreach ($f in @("unity.c", "unity.h", "unity_internals.h")) {
    $dst = Join-Path $outDir $f
    Write-Host "Downloading $f ..."
    Invoke-WebRequest -Uri "$base/$f" -OutFile $dst -UseBasicParsing
}

Write-Host ""
Write-Host "Done. Unity is in: $outDir"
Write-Host "Next step: make all"

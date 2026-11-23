$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $zig=Join-Path $root '.tools\zig-x86_64-windows-0.15.2\zig.exe'
    if(!(Test-Path -LiteralPath 'build/icons.obj')){throw 'Run scripts/build.ps1 first.'}
    & $zig c++ tools/number-preview.cpp build/icons.obj -O2 -std=c++20 -municode -lgdi32 -luser32 -o build/number-preview.exe
    if($LASTEXITCODE){throw 'Preview compilation failed.'}
    & '.\build\number-preview.exe'
    if($LASTEXITCODE){throw 'Private JetBrains font selection or rendering failed.'}
    node scripts/preview-numbers.mjs
    if($LASTEXITCODE){throw 'PNG preview creation failed.'}
} finally { Pop-Location }

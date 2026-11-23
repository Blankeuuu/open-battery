$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $zig = Join-Path $root '.tools\zig-x86_64-windows-0.15.2\zig.exe'
    New-Item -ItemType Directory -Path 'build' -Force | Out-Null
    node scripts/render-icons.mjs
    if ($LASTEXITCODE) { throw 'Icon rendering failed.' }
    & $zig rc /fo build/icons.obj build/icons.rc
    if ($LASTEXITCODE) { throw 'Icon resource compilation failed.' }
    & $zig c++ tests/model-tests.cpp -O2 -std=c++20 -o build/model-tests.exe
    if ($LASTEXITCODE) { throw 'Model compilation failed.' }
    & '.\build\model-tests.exe'
    if ($LASTEXITCODE) { throw 'Model tests failed.' }
    & $zig c++ tests/integration-tests.cpp build/icons.obj -O2 -std=c++20 -lhid -lsetupapi -lshell32 -lole32 -ladvapi32 -lgdi32 -luser32 -luuid -o build/integration-tests.exe
    if ($LASTEXITCODE) { throw 'Integration compilation failed.' }
    & $zig c++ tests/wake-regression.cpp build/icons.obj -O2 -std=c++20 -lhid -lsetupapi -lshell32 -lole32 -ladvapi32 -lgdi32 -luser32 -luuid -o build/wake-regression.exe
    if ($LASTEXITCODE) { throw 'Wake regression compilation failed.' }
    & '.\build\wake-regression.exe'
    if ($LASTEXITCODE) { throw 'Wake regression failed.' }
    & '.\build\integration-tests.exe'
    if ($LASTEXITCODE) { throw 'Integration tests failed.' }
} finally { Pop-Location }

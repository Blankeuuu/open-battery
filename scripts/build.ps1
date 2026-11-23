param([string]$Version = '0.1.1', [switch]$SkipMsi)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    $zig = Join-Path $root '.tools\zig-x86_64-windows-0.15.2\zig.exe'
    if (!(Test-Path -LiteralPath $zig)) { throw 'Run scripts/bootstrap.ps1 first.' }
    New-Item -ItemType Directory -Path 'build' -Force | Out-Null
    node scripts/render-icons.mjs $Version
    if ($LASTEXITCODE) { throw 'Icon rendering failed. Run scripts/bootstrap.ps1.' }
    & $zig rc /fo build/icons.obj build/icons.rc
    if ($LASTEXITCODE) { throw 'Icon resource compilation failed.' }
    & $zig c++ tests/model-tests.cpp -O2 -std=c++20 -o build/model-tests.exe
    if ($LASTEXITCODE) { throw 'Test compilation failed.' }
    & '.\build\model-tests.exe'
    if ($LASTEXITCODE) { throw 'Model tests failed.' }
    & $zig c++ tools/battery-status.cpp -O2 -std=c++20 -lhid -lsetupapi -o build/battery-status.exe
    if ($LASTEXITCODE) { throw 'Status tool compilation failed.' }
    & $zig c++ src/main.cpp build/icons.obj -O2 -std=c++20 -municode '-Wl,--subsystem,windows' -lhid -lsetupapi -lshell32 -lole32 -ladvapi32 -lgdi32 -luser32 -luuid -o build/OpenBattery.exe
    if ($LASTEXITCODE) { throw 'Application compilation failed. Close Open Battery before rebuilding.' }
    if (!$SkipMsi) {
        $wix = Join-Path $root '.tools\wix\wix.exe'
        if (!(Test-Path -LiteralPath $wix)) { throw 'Run scripts/bootstrap.ps1 first.' }
        & $wix build installer/Package.wxs -arch x64 -ext WixToolset.UI.wixext/4.0.6 -d "ProductVersion=$Version" -d "BuildDir=$root\build" -d "SourceDir=$root" -o "build/OpenBattery-$Version-x64.msi"
        if ($LASTEXITCODE) { throw 'MSI build failed.' }
    }
} finally { Pop-Location }

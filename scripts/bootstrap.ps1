$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    New-Item -ItemType Directory -Path '.tools' -Force | Out-Null
    $zig = '.tools\zig-x86_64-windows-0.15.2\zig.exe'
    if (!(Test-Path -LiteralPath $zig)) {
        $archive = '.tools\zig.zip'
        Invoke-WebRequest -Uri 'https://ziglang.org/download/0.15.2/zig-x86_64-windows-0.15.2.zip' -OutFile $archive
        $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($hash -ne '3a0ed1e8799a2f8ce2a6e6290a9ff22e6906f8227865911fb7ddedc3cc14cb0c') { throw 'Zig archive SHA256 mismatch.' }
        Expand-Archive -LiteralPath $archive -DestinationPath '.tools' -Force
    }
    New-Item -ItemType Directory -Path '.tools\icon-tools' -Force | Out-Null
    Copy-Item -LiteralPath 'tools\icon-tools\package.json','tools\icon-tools\package-lock.json' -Destination '.tools\icon-tools'
    npm.cmd ci --prefix .tools/icon-tools --ignore-scripts --no-audit --no-fund
    if ($LASTEXITCODE) { throw 'Icon build tools installation failed.' }
    $env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'
    $env:DOTNET_GENERATE_ASPNET_CERTIFICATE = 'false'
    $env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
    $env:DOTNET_NOLOGO = '1'
    if (!(Test-Path -LiteralPath '.tools\wix\wix.exe')) {
        dotnet tool install wix --version 4.0.6 --tool-path .tools/wix
        if ($LASTEXITCODE) { throw 'Local WiX installation failed.' }
    }
    & '.\.tools\wix\wix.exe' extension add WixToolset.UI.wixext/4.0.6
    if ($LASTEXITCODE) { throw 'WiX UI extension installation failed.' }
} finally { Pop-Location }

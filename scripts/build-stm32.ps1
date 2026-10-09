param(
    [ValidateSet('CM4','CM7')][string]$Core = 'CM4',
    [ValidateSet('Build','Clean','Rebuild')][string]$Action = 'Build',
    [string]$ArmToolchainBin = $env:ARM_TOOLCHAIN_BIN
)
$ErrorActionPreference = 'Stop'
try {
    $projectRoot = Join-Path (Split-Path $PSScriptRoot -Parent) 'Rev 1.01'
    $sourceDir = Join-Path $projectRoot $Core
    if (!(Test-Path (Join-Path $sourceDir 'CMakeLists.txt'))) {
        throw "$Core source project is absent. VID-18 currently builds CM4 only; no CM7 firmware or boot-option changes are supplied."
    }
    if (!$ArmToolchainBin -and !(Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue)) {
        $knownBin = 'C:\Arm GNU Toolchain arm-none-eabi\13.2 Rel1\bin'
        if (Test-Path (Join-Path $knownBin 'arm-none-eabi-gcc.exe')) { $ArmToolchainBin = $knownBin }
    }
    if ($ArmToolchainBin) { $env:Path = "$ArmToolchainBin;$env:Path" }
    foreach ($tool in @('cmake','ninja','arm-none-eabi-gcc','arm-none-eabi-g++')) {
        if (!(Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "$tool was not found. Set ARM_TOOLCHAIN_BIN to the ARM compiler bin folder if needed."
        }
    }
    $buildDir = Join-Path $sourceDir 'build'
    $startTime = Get-Date
    # Always configure first: this creates missing Ninja files and picks up source-list changes.
    # A moved legacy cache must be discarded before configuration.
    $cache = Join-Path $buildDir 'CMakeCache.txt'
    $fresh = $Action -eq 'Rebuild'
    if (Test-Path $cache) {
        $homeLine = Get-Content $cache | Where-Object { $_ -like 'CMAKE_HOME_DIRECTORY:INTERNAL=*' } | Select-Object -First 1
        if ($homeLine) {
            $cachedSource = $homeLine.Substring($homeLine.IndexOf('=') + 1).Replace('/','\').TrimEnd('\')
            if ($cachedSource -ine $sourceDir.Replace('/','\').TrimEnd('\')) { $fresh = $true }
        }
    }
    # Remove only CMake-generated cache/configuration, never source or saved host settings.
    if ($fresh) {
        Remove-Item $cache -Force -ErrorAction SilentlyContinue
        Remove-Item (Join-Path $buildDir 'CMakeFiles') -Recurse -Force -ErrorAction SilentlyContinue
    }
    & cmake -S $sourceDir -B $buildDir -G Ninja '-DCMAKE_BUILD_TYPE=Debug'
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    if ($Action -eq 'Clean') {
        & cmake --build $buildDir --target clean
    } elseif ($Action -eq 'Rebuild') {
        & cmake --build $buildDir --clean-first
    } else {
        & cmake --build $buildDir
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $seconds = ((Get-Date) - $startTime).TotalSeconds
    Write-Host "$Core $Action succeeded in $seconds seconds"
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}

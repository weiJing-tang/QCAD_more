$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$qtRoot = Join-Path $projectRoot 'build/Qt/5.15.2/msvc2019_64'
$application = Join-Path $projectRoot 'build/cmake/Release/QCAD_more.exe'
if (-not (Test-Path -LiteralPath $application)) { throw 'Run scripts/build.ps1 first.' }
$savedPath = $env:PATH
$savedPlugins = $env:QT_QPA_PLATFORM_PLUGIN_PATH
$savedPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = "$qtRoot/bin;$savedPath"
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = "$qtRoot/plugins/platforms"
    $env:QT_QPA_PLATFORM = 'windows'
    & $application
} finally {
    $env:PATH = $savedPath
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = $savedPlugins
    $env:QT_QPA_PLATFORM = $savedPlatform
}

param([switch]$Check)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$qtRoot = Join-Path $projectRoot 'build/Qt/5.15.2/msvc2019_64'
$buildRoot = Join-Path $projectRoot 'build/cmake'
if (-not (Test-Path -LiteralPath "$qtRoot/lib/cmake/Qt5/Qt5Config.cmake")) {
    throw 'Run scripts/setup-qt.ps1 first.'
}
$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($cmakeCommand) { $cmake = $cmakeCommand.Source }
else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ tools and CMake.' }
    $cmake = & $vswhere -latest -products '*' -find 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' | Select-Object -First 1
    if (-not $cmake) { throw 'CMake was not found in Visual Studio.' }
}
& $cmake -S $projectRoot -B $buildRoot "-DCMAKE_PREFIX_PATH=$qtRoot"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build $buildRoot --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
if ($Check) {
    $savedPath = $env:PATH
    $savedPlugins = $env:QT_QPA_PLATFORM_PLUGIN_PATH
    try {
        $env:PATH = "$qtRoot/bin;$savedPath"
        $env:QT_QPA_PLATFORM_PLUGIN_PATH = "$qtRoot/plugins/platforms"
        & (Join-Path (Split-Path $cmake) 'ctest.exe') --test-dir $buildRoot -C Release --output-on-failure --timeout 90
        if ($LASTEXITCODE -ne 0) {
            Get-Content -LiteralPath "$buildRoot/test-results.txt" -ErrorAction SilentlyContinue
            throw 'Workflow verification failed.'
        }
    } finally { $env:PATH = $savedPath; $env:QT_QPA_PLATFORM_PLUGIN_PATH = $savedPlugins }
}
Write-Host 'Run: powershell -ExecutionPolicy Bypass -File scripts/run.ps1'

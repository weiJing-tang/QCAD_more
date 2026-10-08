param([string]$CondaExe = "conda.exe")
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = Join-Path $projectRoot 'build'
$environmentPath = Join-Path $buildRoot 'qt-dev'
$python = Join-Path $environmentPath 'python.exe'
$qtRoot = Join-Path $buildRoot 'Qt'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
if (-not (Test-Path -LiteralPath $python)) {
    & $CondaExe create --prefix $environmentPath python=3.11 pip --yes
    if ($LASTEXITCODE -ne 0) { throw 'Could not create isolated qt-dev environment.' }
}
& $python -m pip install --disable-pip-version-check aqtinstall==3.3.0
if ($LASTEXITCODE -ne 0) { throw 'Could not install the Qt download tool.' }
if (-not (Test-Path -LiteralPath (Join-Path $qtRoot '5.15.2/msvc2019_64/lib/cmake/Qt5/Qt5Config.cmake'))) {
    Push-Location $buildRoot
    try {
        & $python -m aqt install-qt windows desktop 5.15.2 win64_msvc2019_64 --archives qtbase --outputdir $qtRoot
        if ($LASTEXITCODE -ne 0) { throw 'Qt download failed.' }
    } finally { Pop-Location }
}
Write-Host "Qt SDK: $qtRoot"
Write-Host "Isolated tools: $environmentPath"

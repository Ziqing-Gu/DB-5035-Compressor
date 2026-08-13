$ErrorActionPreference = "Stop"

$projectRoot = Join-Path $PSScriptRoot "vst3-juce"
$fetchBuildDir = Join-Path $projectRoot "build-vs"
$localBuildDir = Join-Path $projectRoot "build-local"
$downloadedJuce = Join-Path $fetchBuildDir "_deps\juce-src"
$vsRoot = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
$vcvars = Join-Path $vsRoot "VC\Auxiliary\Build\vcvars64.bat"
$cmake = Join-Path $vsRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at $vcvars"
}

if (-not (Test-Path $cmake)) {
    throw "cmake.exe not found at $cmake"
}

$machinePath = [Environment]::GetEnvironmentVariable("Path", "Machine")
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
$cleanPath = (($machinePath, $userPath) | Where-Object { $_ }) -join ";"

[Environment]::SetEnvironmentVariable("PATH", $null, "Process")
[Environment]::SetEnvironmentVariable("Path", $cleanPath, "Process")

Set-Location $projectRoot

if (Test-Path (Join-Path $downloadedJuce "CMakeLists.txt")) {
    $buildDir = $localBuildDir
    $configure = "`"$vcvars`" && `"$cmake`" -S . -B `"$buildDir`" -G `"Visual Studio 17 2022`" -A x64 -DDB5035_FETCH_JUCE=OFF -DDB5035_JUCE_PATH=`"$downloadedJuce`""
} else {
    $buildDir = $fetchBuildDir
    $configure = "`"$vcvars`" && `"$cmake`" -S . -B `"$buildDir`" -G `"Visual Studio 17 2022`" -A x64"
}

$build = "`"$vcvars`" && `"$cmake`" --build `"$buildDir`" --target DB5035Compressor_VST3 --config Release"

cmd.exe /d /c $configure
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

cmd.exe /d /c $build
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$artifact = Join-Path $buildDir "DB5035Compressor_artefacts\Release\VST3\DB-5035 Qing Compressor.vst3"

if (-not (Test-Path $artifact)) {
    throw "DB-5035 Qing Compressor VST3 was built, but the expected bundle was not found at $artifact"
}

$distDir = Join-Path $PSScriptRoot "dist"
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$package = Join-Path $distDir "DB-5035-Qing-Compressor-1.1.0-Windows-x64-$stamp.zip"
New-Item -ItemType Directory -Force -Path $distDir | Out-Null
Compress-Archive -Path $artifact -DestinationPath $package

Write-Host ""
Write-Host "VST3 bundle:        $artifact" -ForegroundColor Green
Write-Host "Portable package:     $package" -ForegroundColor Green
exit 0

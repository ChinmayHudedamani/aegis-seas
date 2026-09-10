$ErrorActionPreference = "Stop"

$toolPath = "$((Get-Item .).FullName)\tools\w64devkit\bin"
if (Test-Path $toolPath) {
    $env:PATH = "$toolPath;" + $env:PATH
}

$cxx = "g++"
$cxxflags = @("-std=c++20", "-O3", "-Wall", "-Wextra", "-Iinclude", "-fopenmp")

$coreSrcs = @(
    "src/physics/calibrator.cpp",
    "src/physics/speckle_filter.cpp",
    "src/physics/polarimetry.cpp",
    "src/nn/layers.cpp",
    "src/nn/unet.cpp",
    "src/nn/sliding_window.cpp",
    "src/geo/slick_analyzer.cpp",
    "src/geo/geojson_writer.cpp",
    "src/simulation/synthetic_scene.cpp",
    "src/drift/lagrangian_drift.cpp",
    "src/attribution/cfar_detector.cpp",
    "src/attribution/ais_engine.cpp",
    "src/io/ais_parser.cpp",
    "src/pipeline/pipeline_orchestrator.cpp"
)

New-Item -ItemType Directory -Force -Path "bin" | Out-Null
New-Item -ItemType Directory -Force -Path "build" | Out-Null

$coreObjs = @()
foreach ($src in $coreSrcs) {
    $rel = $src -replace "^src/", ""
    $obj = "build/" + ($rel -replace "\.cpp$", ".o")
    $objDir = Split-Path -Parent $obj
    if (-not (Test-Path $objDir)) {
        New-Item -ItemType Directory -Force -Path $objDir | Out-Null
    }
    
    $needsBuild = $true
    if ((Test-Path $obj) -and (Test-Path $src)) {
        if ((Get-Item $obj).LastWriteTime -ge (Get-Item $src).LastWriteTime) {
            $needsBuild = $false
        }
    }
    
    if ($needsBuild) {
        Write-Host "Compiling $src -> $obj..." -ForegroundColor Cyan
        & $cxx @cxxflags -c $src -o $obj
        if ($LASTEXITCODE -ne 0) { throw "Compilation failed for $src" }
    }
    $coreObjs += $obj
}

# Compile main.cpp
$mainObj = "build/main.o"
Write-Host "Compiling src/main.cpp -> $mainObj..." -ForegroundColor Cyan
& $cxx @cxxflags -c "src/main.cpp" -o $mainObj
if ($LASTEXITCODE -ne 0) { throw "Compilation failed for src/main.cpp" }

# Link main target
$target = "bin/sar_oil_detector.exe"
Write-Host "Linking $target..." -ForegroundColor Green
& $cxx @cxxflags @coreObjs $mainObj -o $target
if ($LASTEXITCODE -ne 0) { throw "Linking failed for $target" }

# Compile test_stress_suite.cpp
$testObjDir = "build/tests"
if (-not (Test-Path $testObjDir)) { New-Item -ItemType Directory -Force -Path $testObjDir | Out-Null }
$testObj = "build/tests/test_stress_suite.o"
Write-Host "Compiling tests/red_team/test_stress_suite.cpp -> $testObj..." -ForegroundColor Cyan
& $cxx @cxxflags -c "tests/red_team/test_stress_suite.cpp" -o $testObj
if ($LASTEXITCODE -ne 0) { throw "Compilation failed for test_stress_suite.cpp" }

# Link test target
$testTarget = "bin/test_stress_suite.exe"
Write-Host "Linking $testTarget..." -ForegroundColor Green
& $cxx @cxxflags @coreObjs $testObj -o $testTarget
if ($LASTEXITCODE -ne 0) { throw "Linking failed for $testTarget" }

Write-Host "[OK] AEGIS-SEAS Build Succeeded! Targets: $target, $testTarget" -ForegroundColor Yellow

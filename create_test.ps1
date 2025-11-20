param(
    [string]$TargetName,
    [string]$FilePath
)

# --- Helpers ---
function Convert-SnakeToPascal {
    param([string]$snake)
    $parts = $snake -replace '\.cpp$', '' -split '_'
    $pascal = ($parts | ForEach-Object { $_.Substring(0,1).ToUpper() + $_.Substring(1).ToLower() }) -join ''
    return $pascal
}

function Convert-PascalToSnake {
    param([string]$pascal)
    $snake = $pascal -creplace '([A-Z])', '_$1'
    $snake = $snake.TrimStart('_').ToLower()
    return "$snake.cpp"
}

# --- Argument Logic ---
if (-not $TargetName -and -not $FilePath) {
    Write-Host "Error: Provide -TargetName or -FilePath" -ForegroundColor Red; exit 1
}
if ($FilePath -and -not $TargetName) {
    $TargetName = Convert-SnakeToPascal (Split-Path $FilePath -Leaf)
    Write-Host "Derived TargetName: $TargetName" -ForegroundColor Cyan
}
if ($TargetName -and -not $FilePath) {
    $FilePath = "test/basic/$((Convert-PascalToSnake $TargetName))"
    Write-Host "Derived FilePath: $FilePath" -ForegroundColor Cyan
}
$FilePath = $FilePath -replace '\\', '/'

# --- File Creation ---
if (-not (Test-Path "template_test.cpp")) {
    New-Item -ItemType File -Path $FilePath -Force | Out-Null
    Write-Warning "Template not found. Created empty file."
} else {
    $parent = Split-Path $FilePath -Parent
    if (-not (Test-Path $parent)) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
    if (Test-Path $FilePath) { Write-Error "File exists: $FilePath"; exit 1 }
    Copy-Item "template_test.cpp" $FilePath
}

# --- CMake Injection (Priority Fallback) ---
$cmakeFile = "CMakeLists.txt"
if (-not (Test-Path $cmakeFile)) { Write-Error "No CMakeLists.txt"; exit 1 }

$lines = Get-Content $cmakeFile -Encoding UTF8 | ForEach-Object { $_ } # array copy
if ($lines -match "add_executable\($TargetName") { Write-Error "Target exists"; exit 1 }

$newLines = @()
$exeAdded = $false
$confAdded = $false

# 1. Handle add_executable
if ($lines -match "# ADD_EXECUTABLE_MARKER") {
    # MARKER FOUND: Insert after marker
    foreach ($line in $lines) {
        $newLines += $line
        if ($line -match "# ADD_EXECUTABLE_MARKER") {
            $newLines += "add_executable($TargetName $FilePath)"
            $exeAdded = $true
        }
    }
} else {
    # MARKER MISSING: Insert before function definition
    foreach ($line in $lines) {
        if ($line -match "^function\(configure_test_target") {
            $newLines += "add_executable($TargetName $FilePath)"
            $exeAdded = $true
        }
        $newLines += $line
    }
}
$lines = $newLines # Update state for next pass

# 2. Handle configure_test_target
$newLines = @()
if ($lines -match "# CONFIGURE_TARGET_MARKER") {
    # MARKER FOUND: Insert after marker
    foreach ($line in $lines) {
        $newLines += $line
        if ($line -match "# CONFIGURE_TARGET_MARKER") {
            $newLines += "configure_test_target($TargetName)"
            $confAdded = $true
        }
    }
} else {
    # MARKER MISSING: Append to end
    $newLines = $lines
    $newLines += "configure_test_target($TargetName)"
    $confAdded = $true
}

$newLines | Set-Content $cmakeFile -Encoding UTF8

# --- Summary ---
Write-Host ""
Write-Host "Test created successfully!" -ForegroundColor Green
Write-Host "  Target: $TargetName" -ForegroundColor Cyan
Write-Host "  File:   $FilePath" -ForegroundColor Cyan
if ($exeAdded -and $confAdded) {
    Write-Host "  CMake:  Updated successfully" -ForegroundColor Green
}
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. cmake -B build"
Write-Host "  2. cmake --build build --target $TargetName"

param(
    [string]$TargetName,
    [string]$FilePath
)

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

# Validate and derive arguments
if (-not $TargetName -and -not $FilePath) {
    Write-Host "Error: At least one of -TargetName or -FilePath must be provided" -ForegroundColor Red
    Write-Host ""
    Write-Host "Usage:"
    Write-Host "  .\remove_test.ps1 -TargetName MyTest"
    Write-Host "  .\remove_test.ps1 -FilePath test/basic/my_test.cpp"
    Write-Host "  .\remove_test.ps1 -TargetName MyTest -FilePath test/basic/my_test.cpp"
    exit 1
}

if ($FilePath -and -not $TargetName) {
    $fileName = Split-Path $FilePath -Leaf
    $TargetName = Convert-SnakeToPascal $fileName
    Write-Host "Derived TargetName: $TargetName" -ForegroundColor Cyan
}

if ($TargetName -and -not $FilePath) {
    # Try to find the file path from CMakeLists.txt
    $cmakeFile = "CMakeLists.txt"
    if (Test-Path $cmakeFile) {
        $content = Get-Content $cmakeFile -Raw
        if ($content -match "add_executable\($TargetName\s+([^\)]+)\)") {
            $FilePath = $matches[1].Trim()
            Write-Host "Found FilePath in CMakeLists.txt: $FilePath" -ForegroundColor Cyan
        } else {
            # Derive from target name
            $fileName = Convert-PascalToSnake $TargetName
            $FilePath = "test/basic/$fileName"
            Write-Host "Derived FilePath: $FilePath" -ForegroundColor Yellow
        }
    }
}

# Ensure the file path is relative to project root
$FilePath = $FilePath -replace '\\', '/'

# Check if CMakeLists.txt exists
$cmakeFile = "CMakeLists.txt"
if (-not (Test-Path $cmakeFile)) {
    Write-Host "Error: CMakeLists.txt not found" -ForegroundColor Red
    exit 1
}

$content = Get-Content $cmakeFile -Raw
$modified = $false

# Check if target exists in CMakeLists.txt
if ($content -notmatch "add_executable\($TargetName\s") {
    Write-Host "Warning: Target '$TargetName' not found in CMakeLists.txt" -ForegroundColor Yellow
} else {
    # Remove add_executable line
    $lines = $content -split "`r?`n"
    $newLines = @()
    
    foreach ($line in $lines) {
        if ($line -notmatch "^add_executable\($TargetName\s") {
            $newLines += $line
        } else {
            Write-Host "Removed from CMakeLists.txt: $line" -ForegroundColor Green
            $modified = $true
        }
    }
    
    $content = $newLines -join "`n"
    $lines = $content -split "`r?`n"
    $newLines = @()
    
    # Remove configure_test_target line
    foreach ($line in $lines) {
        if ($line -notmatch "^configure_test_target\($TargetName\)") {
            $newLines += $line
        } else {
            Write-Host "Removed from CMakeLists.txt: $line" -ForegroundColor Green
            $modified = $true
        }
    }
    
    if ($modified) {
        $newLines -join "`n" | Set-Content $cmakeFile -NoNewline
    }
}

# Check if test file exists and remove it
if (Test-Path $FilePath) {
    Remove-Item $FilePath
    Write-Host "Deleted test file: $FilePath" -ForegroundColor Green
    $modified = $true
} else {
    Write-Host "Warning: Test file not found: $FilePath" -ForegroundColor Yellow
}

if ($modified) {
    Write-Host ""
    Write-Host "Test removed successfully!" -ForegroundColor Green
    Write-Host "  Target: $TargetName" -ForegroundColor Cyan
    Write-Host "  File: $FilePath" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "No changes made. Test may not exist." -ForegroundColor Yellow
}

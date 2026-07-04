# ============================================================================
# DEPLOY SCRIPT FOR FACEID APPLICATION (SQLite & Ucrt64)
# Run instructions:
#   1. Open PowerShell in the project directory
#   2. Run command: ./deploy_vison.ps1
# ============================================================================

# Configuration paths
$dest = "vison"
$qtBin = "C:\Qt\6.11.1\mingw_64\bin"
$msysBin = "C:\msys64\ucrt64\bin"
$objdump = "C:\Qt\Tools\mingw1310_64\bin\objdump.exe"

# 1. Initialize output directory
Write-Host "1. Initializing destination folder '$dest'..." -ForegroundColor Cyan
if (Test-Path $dest) {
    Remove-Item -Recurse -Force $dest
}
New-Item -ItemType Directory -Path $dest | Out-Null
New-Item -ItemType Directory -Path "$dest\models" | Out-Null

# 2. Copy main executable
Write-Host "2. Copying main executable..." -ForegroundColor Cyan
$exePath = "build\FacieID_UtherDashbbord.exe"
if (Test-Path $exePath) {
    Copy-Item -Path $exePath -Destination "$dest\FacieID_UtherDashbbord.exe" -Force
} else {
    Write-Error "FacieID_UtherDashbbord.exe not found! Please build the project first."
    exit
}

# 3. Run windeployqt
Write-Host "3. Gathering Qt dependencies (windeployqt)..." -ForegroundColor Cyan
if (Test-Path "$qtBin\windeployqt.exe") {
    & "$qtBin\windeployqt.exe" --compiler-runtime "$dest\FacieID_UtherDashbbord.exe"
} else {
    Write-Warning "windeployqt.exe not found at $qtBin."
}

# Sync GCC runtime DLLs from MSYS2
Write-Host "-> Syncing GCC runtime from MSYS2..." -ForegroundColor Cyan
Copy-Item -Path "$msysBin\libstdc++-6.dll", "$msysBin\libgcc_s_seh-1.dll", "$msysBin\libwinpthread-1.dll" -Destination $dest -Force

# 4. Scan dependencies recursively for OpenCV & MSYS2 DLLs
Write-Host "4. Analyzing and copying OpenCV & MSYS2 dependencies..." -ForegroundColor Cyan
if (Test-Path $objdump) {
    $systemDlls = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $commonSysDlls = @("kernel32.dll", "user32.dll", "gdi32.dll", "winspool.dll", "comdlg32.dll", "advapi32.dll", 
                       "shell32.dll", "ole32.dll", "oleaut32.dll", "uuid.dll", "odbc32.dll", "odbccp32.dll", 
                       "ws2_32.dll", "msvcrt.dll", "shlwapi.dll", "dnsapi.dll", "iphlpapi.dll", "crypt32.dll", 
                       "secur32.dll", "winmm.dll", "comctl32.dll", "mpr.dll", "userenv.dll", "bcrypt.dll", 
                       "dbghelp.dll", "setupapi.dll", "version.dll", "imm32.dll", "usp10.dll", "gdiplus.dll",
                       "oleacc.dll", "uxtheme.dll", "dwmapi.dll", "netapi32.dll", "opengl32.dll", "glu32.dll",
                       "d3d11.dll", "dxgi.dll", "dxguid.dll", "d3d12.dll", "d3d9.dll")
    foreach ($sysDll in $commonSysDlls) {
        $systemDlls.Add($sysDll) | Out-Null
    }

    $toProcess = [System.Collections.Generic.Queue[string]]::new()
    $processed = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $toProcess.Enqueue("$dest\FacieID_UtherDashbbord.exe")

    while ($toProcess.Count -gt 0) {
        $file = $toProcess.Dequeue()
        $fileName = Split-Path $file -Leaf
        if ($processed.Contains($fileName)) { continue }
        $processed.Add($fileName) | Out-Null

        $imports = & $objdump -p $file | Select-String "DLL Name" | ForEach-Object {
            $_.Line.Split(":")[-1].Trim()
        }

        foreach ($import in $imports) {
            if ($systemDlls.Contains($import) -or $import.StartsWith("api-ms-win-") -or $import.StartsWith("ext-ms-")) {
                continue
            }

            $targetPath = "$dest\$import"
            if (-not (Test-Path $targetPath)) {
                $found = $false
                $searchPaths = @("$msysBin\$import", "$qtBin\$import")
                foreach ($sp in $searchPaths) {
                    if (Test-Path $sp) {
                        Copy-Item -Path $sp -Destination $dest -Force
                        $toProcess.Enqueue($targetPath)
                        $found = $true
                        break
                    }
                }
            } else {
                $toProcess.Enqueue($targetPath)
            }
        }
    }
    Write-Host "-> Completed scanning and copying DLL dependencies." -ForegroundColor Green
} else {
    Write-Warning "objdump.exe not found."
}


# 4b. Force-copy ALL libabsl DLLs (abseil-cpp, dependency of protobuf/OpenCV DNN)
Write-Host "4b. Force-copying all libabsl (abseil-cpp) DLLs..." -ForegroundColor Cyan
Get-ChildItem "$msysBin\libabsl*" | ForEach-Object {
    Copy-Item # ============================================================================
# DEPLOY SCRIPT FOR FACEID APPLICATION (SQLite & Ucrt64)
# Run instructions:
#   1. Open PowerShell in the project directory
#   2. Run command: ./deploy_vison.ps1
# ============================================================================

# Configuration paths
$dest = "vison"
$qtBin = "C:\Qt\6.11.1\mingw_64\bin"
$msysBin = "C:\msys64\ucrt64\bin"
$objdump = "C:\Qt\Tools\mingw1310_64\bin\objdump.exe"

# 1. Initialize output directory
Write-Host "1. Initializing destination folder '$dest'..." -ForegroundColor Cyan
if (Test-Path $dest) {
    Remove-Item -Recurse -Force $dest
}
New-Item -ItemType Directory -Path $dest | Out-Null
New-Item -ItemType Directory -Path "$dest\models" | Out-Null

# 2. Copy main executable
Write-Host "2. Copying main executable..." -ForegroundColor Cyan
$exePath = "build\FacieID_UtherDashbbord.exe"
if (Test-Path $exePath) {
    Copy-Item -Path $exePath -Destination "$dest\FacieID_UtherDashbbord.exe" -Force
} else {
    Write-Error "FacieID_UtherDashbbord.exe not found! Please build the project first."
    exit
}

# 3. Run windeployqt
Write-Host "3. Gathering Qt dependencies (windeployqt)..." -ForegroundColor Cyan
if (Test-Path "$qtBin\windeployqt.exe") {
    & "$qtBin\windeployqt.exe" --compiler-runtime "$dest\FacieID_UtherDashbbord.exe"
} else {
    Write-Warning "windeployqt.exe not found at $qtBin."
}

# Sync GCC runtime DLLs from MSYS2
Write-Host "-> Syncing GCC runtime from MSYS2..." -ForegroundColor Cyan
Copy-Item -Path "$msysBin\libstdc++-6.dll", "$msysBin\libgcc_s_seh-1.dll", "$msysBin\libwinpthread-1.dll" -Destination $dest -Force

# 4. Scan dependencies recursively for OpenCV & MSYS2 DLLs
Write-Host "4. Analyzing and copying OpenCV & MSYS2 dependencies..." -ForegroundColor Cyan
if (Test-Path $objdump) {
    $systemDlls = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $commonSysDlls = @("kernel32.dll", "user32.dll", "gdi32.dll", "winspool.dll", "comdlg32.dll", "advapi32.dll", 
                       "shell32.dll", "ole32.dll", "oleaut32.dll", "uuid.dll", "odbc32.dll", "odbccp32.dll", 
                       "ws2_32.dll", "msvcrt.dll", "shlwapi.dll", "dnsapi.dll", "iphlpapi.dll", "crypt32.dll", 
                       "secur32.dll", "winmm.dll", "comctl32.dll", "mpr.dll", "userenv.dll", "bcrypt.dll", 
                       "dbghelp.dll", "setupapi.dll", "version.dll", "imm32.dll", "usp10.dll", "gdiplus.dll",
                       "oleacc.dll", "uxtheme.dll", "dwmapi.dll", "netapi32.dll", "opengl32.dll", "glu32.dll",
                       "d3d11.dll", "dxgi.dll", "dxguid.dll", "d3d12.dll", "d3d9.dll")
    foreach ($sysDll in $commonSysDlls) {
        $systemDlls.Add($sysDll) | Out-Null
    }

    $toProcess = [System.Collections.Generic.Queue[string]]::new()
    $processed = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $toProcess.Enqueue("$dest\FacieID_UtherDashbbord.exe")

    while ($toProcess.Count -gt 0) {
        $file = $toProcess.Dequeue()
        $fileName = Split-Path $file -Leaf
        if ($processed.Contains($fileName)) { continue }
        $processed.Add($fileName) | Out-Null

        $imports = & $objdump -p $file | Select-String "DLL Name" | ForEach-Object {
            $_.Line.Split(":")[-1].Trim()
        }

        foreach ($import in $imports) {
            if ($systemDlls.Contains($import) -or $import.StartsWith("api-ms-win-") -or $import.StartsWith("ext-ms-")) {
                continue
            }

            $targetPath = "$dest\$import"
            if (-not (Test-Path $targetPath)) {
                $found = $false
                $searchPaths = @("$msysBin\$import", "$qtBin\$import")
                foreach ($sp in $searchPaths) {
                    if (Test-Path $sp) {
                        Copy-Item -Path $sp -Destination $dest -Force
                        $toProcess.Enqueue($targetPath)
                        $found = $true
                        break
                    }
                }
            } else {
                $toProcess.Enqueue($targetPath)
            }
        }
    }
    Write-Host "-> Completed scanning and copying DLL dependencies." -ForegroundColor Green
} else {
    Write-Warning "objdump.exe not found."
}

# 5. Copy AI models
Write-Host "5. Copying AI models folder..." -ForegroundColor Cyan
if (Test-Path "models") {
    Copy-Item -Path "models\*" -Destination "$dest\models" -Recurse -Force
}

# 6. Copy assets & access folders
Write-Host "6. Copying assets & access folders..." -ForegroundColor Cyan
if (Test-Path "assets") {
    Copy-Item -Path "assets" -Destination $dest -Recurse -Force
}
if (Test-Path "access") {
    Copy-Item -Path "access" -Destination $dest -Recurse -Force
}

Write-Host "==========================================" -ForegroundColor Green
Write-Host "DEPLOYMENT COMPLETE! Folder '$dest' is ready." -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
.FullName -Destination $dest -Force
}
Write-Host "-> libabsl DLLs copied." -ForegroundColor Green
# 5. Copy AI models
Write-Host "5. Copying AI models folder..." -ForegroundColor Cyan
if (Test-Path "models") {
    Copy-Item -Path "models\*" -Destination "$dest\models" -Recurse -Force
}

# 6. Copy assets & access folders
Write-Host "6. Copying assets & access folders..." -ForegroundColor Cyan
if (Test-Path "assets") {
    Copy-Item -Path "assets" -Destination $dest -Recurse -Force
}
if (Test-Path "access") {
    Copy-Item -Path "access" -Destination $dest -Recurse -Force
}

Write-Host "==========================================" -ForegroundColor Green
Write-Host "DEPLOYMENT COMPLETE! Folder '$dest' is ready." -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green


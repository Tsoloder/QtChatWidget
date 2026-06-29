# QtChatWidget 一键运行脚本
# 用法:
#   .\run.ps1              # 运行主程序 (默认)
#   .\run.ps1 chat         # 运行主程序
#   .\run.ps1 shot         # 运行截图程序
#   .\run.ps1 build        # 重新构建
#   .\run.ps1 rebuild      # 清理后重新构建
#   .\run.ps1 all          # 构建 + 截图 + 打开图片

param(
    [Parameter(Position = 0)]
    [ValidateSet("chat", "shot", "build", "rebuild", "all", "menu")]
    [string]$Action = "menu"
)

# ===== 路径配置 (如 Qt 安装位置变化, 修改此处) =====
$ProjectRoot   = $PSScriptRoot
$BuildDir      = Join-Path $ProjectRoot "build"
$ReleaseDir    = Join-Path $BuildDir "Release"
$QtBinPath     = "D:\Application\Anaconda\Library\bin"
$ChatExe       = Join-Path $ReleaseDir "ClineLikeChat.exe"
$ShotExe       = Join-Path $ReleaseDir "SkillScreenshot.exe"
$ShotOutput    = Join-Path $ProjectRoot "skill_features.png"

# ===== 设置 Qt DLL 路径 =====
if (Test-Path $QtBinPath) {
    $env:PATH = "$QtBinPath;" + $env:PATH
} else {
    Write-Warning "Qt bin 路径不存在: $QtBinPath"
    Write-Warning "请编辑本脚本顶部的 `$QtBinPath 变量"
}

function Show-Banner {
    Write-Host ""
    Write-Host "  ============================================" -ForegroundColor Cyan
    Write-Host "   QtChatWidget Skill 系统一键运行脚本" -ForegroundColor Cyan
    Write-Host "  ============================================" -ForegroundColor Cyan
    Write-Host ""
}

function Invoke-Build([string]$target = "all", [switch]$Clean) {
    if (-not (Test-Path $BuildDir)) {
        Write-Host "[1/3] CMake 配置..." -ForegroundColor Yellow
        cmake -S $ProjectRoot -B $BuildDir -G "Visual Studio 18 2026" -A x64 `
              -DQt5_DIR="D:/Application/Anaconda/Library/lib/cmake/Qt5" `
              -DCMAKE_PREFIX_PATH="D:/Application/Anaconda/Library/lib/cmake"
        if ($LASTEXITCODE -ne 0) { Write-Error "CMake 配置失败"; exit 1 }
    }
    if ($Clean) {
        Write-Host "[1/3] 清理旧构建..." -ForegroundColor Yellow
        cmake --build $BuildDir --target clean --config Release 2>$null
    }
    Write-Host "[2/3] 构建 $target..." -ForegroundColor Yellow
    if ($target -eq "all") {
        cmake --build $BuildDir --config Release
    } else {
        cmake --build $BuildDir --target $target --config Release
    }
    if ($LASTEXITCODE -ne 0) { Write-Error "构建失败"; exit 1 }
    Write-Host "[3/3] 构建完成" -ForegroundColor Green
}

function Invoke-Chat {
    if (-not (Test-Path $ChatExe)) {
        Write-Host "主程序不存在, 先执行构建..." -ForegroundColor Yellow
        Invoke-Build "ClineLikeChat"
    }
    Write-Host "启动主程序: $ChatExe" -ForegroundColor Green
    & $ChatExe
}

function Invoke-Shot {
    if (-not (Test-Path $ShotExe)) {
        Write-Host "截图程序不存在, 先执行构建..." -ForegroundColor Yellow
        Invoke-Build "SkillScreenshot"
    }
    Write-Host "运行截图程序..." -ForegroundColor Green
    & $ShotExe
    if (Test-Path $ShotOutput) {
        Write-Host "截图已保存: $ShotOutput" -ForegroundColor Green
    }
}

function Invoke-All {
    Invoke-Build "SkillScreenshot"
    Invoke-Shot
    Write-Host "打开截图..." -ForegroundColor Green
    Start-Process $ShotOutput
}

function Show-Menu {
    Show-Banner
    Write-Host "  [1] 运行主程序 (交互式聊天界面)" -ForegroundColor White
    Write-Host "  [2] 运行截图程序 (生成 skill_features.png)" -ForegroundColor White
    Write-Host "  [3] 构建全部目标" -ForegroundColor White
    Write-Host "  [4] 构建 + 截图 + 自动打开" -ForegroundColor White
    Write-Host "  [5] 清理后重新构建" -ForegroundColor White
    Write-Host "  [Q] 退出" -ForegroundColor White
    Write-Host ""
    $choice = Read-Host "请选择"
    switch ($choice) {
        "1" { Invoke-Chat }
        "2" { Invoke-Shot }
        "3" { Invoke-Build "all" }
        "4" { Invoke-All }
        "5" { Invoke-Build "all" -Clean }
        { $_ -in "q", "Q", "" } { return }
        default { Write-Host "无效选择" -ForegroundColor Red }
    }
}

# ===== 主入口 =====
switch ($Action) {
    "chat"    { Invoke-Chat }
    "shot"    { Invoke-Shot }
    "build"   { Invoke-Build "all" }
    "rebuild" { Invoke-Build "all" -Clean }
    "all"     { Invoke-All }
    "menu"    { Show-Menu }
}

# Copies rainmeter/ToDo into the Rainmeter skins directory and the CursorPos plugin into
# %APPDATA%\Rainmeter\Plugins, deactivates the retired ToDo\Tasks and ToDo\Chat skins, and activates
# ToDo\Main and ToDo\Mascot.
# Generates @Resources/paths.inc (UTF-16 LE, required by Rainmeter for non-ASCII paths).
# An existing @Resources/ui_state.inc in the destination is kept; keys it lacks are added with their defaults.
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$Config = "",
    # Comma-separated skin names; powershell -File passes "A,B" as one string.
    [string]$Activate = "Main,Mascot",
    [string]$Deactivate = "Tasks,Chat"
)

$ErrorActionPreference = "Stop"

$exe = Join-Path $RepoRoot "build\Release\todo.exe"
if ($Config -eq "") { $Config = Join-Path $RepoRoot "todo.ini" }
if (-not (Test-Path -LiteralPath $exe)) { throw "todo.exe not found: $exe (build it first)" }
if (-not (Test-Path -LiteralPath $Config)) { throw "config not found: $Config" }

$rainmeterIni = Join-Path $env:APPDATA "Rainmeter\Rainmeter.ini"
$skinLine = Get-Content -LiteralPath $rainmeterIni | Where-Object { $_ -like "SkinPath=*" } | Select-Object -First 1
if (-not $skinLine) { throw "SkinPath not found in $rainmeterIni" }
$skinsDir = $skinLine.Substring("SkinPath=".Length)
$source = Join-Path $RepoRoot "rainmeter\ToDo"
$dest = Join-Path $skinsDir "ToDo"
$uiState = Join-Path $dest "@Resources\ui_state.inc"
$keptUiState = $null
if (Test-Path -LiteralPath $uiState) { $keptUiState = [IO.File]::ReadAllBytes($uiState) }

New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item -Path (Join-Path $source "*") -Destination $dest -Recurse -Force
if ($null -ne $keptUiState) {
    $defaults = Get-Content -LiteralPath $uiState
    [IO.File]::WriteAllBytes($uiState, $keptUiState)
    $keptKeys = @{}
    foreach ($line in (Get-Content -LiteralPath $uiState)) {
        if ($line -match "^([^=;\[]+)=") { $keptKeys[$Matches[1].Trim()] = $true }
    }
    $missing = @($defaults | Where-Object { $_ -match "^([^=;\[]+)=" -and -not $keptKeys.ContainsKey($Matches[1].Trim()) })
    if ($missing.Count -gt 0) {
        $text = [IO.File]::ReadAllText($uiState)
        if ($text.Length -gt 0 -and -not $text.EndsWith("`n")) { $text += "`r`n" }
        $text += ($missing -join "`r`n") + "`r`n"
        [IO.File]::WriteAllText($uiState, $text, [Text.Encoding]::ASCII)
        Write-Output "ui_state.inc: added $($missing -join ', ')"
    }
}

# The mascot reads the cursor through build\Release\CursorPos.dll (plugins/cursor_pos) and the user's idle
# time, full-screen apps, and the ToDo window through build\Release\DesktopState.dll (plugins/desktop_state).
$pluginsDir = Join-Path $env:APPDATA "Rainmeter\Plugins"
New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null
foreach ($pluginName in "CursorPos.dll", "DesktopState.dll") {
    $plugin = Join-Path $RepoRoot "build\Release\$pluginName"
    if (-not (Test-Path -LiteralPath $plugin)) { throw "$pluginName not found: $plugin (build it first)" }
    try {
        Copy-Item -LiteralPath $plugin -Destination $pluginsDir -Force
    } catch {
        Write-Output "$pluginName is loaded by Rainmeter; kept the installed copy (restart Rainmeter to update it)"
    }
}

$paths ="[Variables]`r`nTodoExe=$exe`r`nTodoConfig=$((Resolve-Path -LiteralPath $Config).Path)`r`n"
[IO.File]::WriteAllText((Join-Path $dest "@Resources\paths.inc"), $paths, [Text.Encoding]::Unicode)

$rainmeter = "C:\Program Files\Rainmeter\Rainmeter.exe"
# Errors are read from the lines Rainmeter logs after this point.
$rainmeterLog = Join-Path $env:APPDATA "Rainmeter\Rainmeter.log"
$logLinesBefore = if (Test-Path -LiteralPath $rainmeterLog) { @(Get-Content -LiteralPath $rainmeterLog).Count } else { 0 }
& $rainmeter "!RefreshApp"
Start-Sleep -Seconds 2
foreach ($name in ($Deactivate -split ",")) {
    $name = $name.Trim()
    if ($name -ne "") { & $rainmeter "!DeactivateConfig" "ToDo\$name" }
}
foreach ($name in ($Activate -split ",")) {
    $name = $name.Trim()
    & $rainmeter "!ActivateConfig" "ToDo\$name" "$name.ini"
}

Write-Output "installed: $dest"
Write-Output "todo.exe:  $exe"
Write-Output "config:    $((Resolve-Path -LiteralPath $Config).Path)"

# A skin that fails to load (an invalid .ini value, a Lua syntax error, too many upvalues) only shows up in
# Rainmeter.log. Lua errors are logged without the skin's name, so they are matched by this skin's script
# file names.
Start-Sleep -Seconds 4
$scriptNames = @(Get-ChildItem -LiteralPath (Join-Path $source "@Resources\Scripts") -Filter *.lua |
    ForEach-Object { [regex]::Escape($_.Name) })
$scriptPattern = "Script: (" + ($scriptNames -join "|") + "):"
$errors = @()
if (Test-Path -LiteralPath $rainmeterLog) {
    $errors = @(Get-Content -LiteralPath $rainmeterLog | Select-Object -Skip $logLinesBefore |
        Where-Object { $_ -match "^ERRO" -and ($_ -match "ToDo\\" -or $_ -match $scriptPattern) })
}
if ($errors.Count -gt 0) {
    Write-Output "Rainmeter reported errors while loading the ToDo skins:"
    $errors | ForEach-Object { Write-Output "  $_" }
    exit 1
}
Write-Output "Rainmeter log: no errors from the ToDo skins"

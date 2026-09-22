param([string]$BuildDir = "build", [string]$Label = "smoke")
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $root "$BuildDir/TaskbarFishing.exe"
$log = Join-Path $root "$BuildDir/$Label.log"
$errLog = Join-Path $root "$BuildDir/$Label.error.log"
$process = Start-Process -FilePath $exe -ArgumentList "--smoke-test" -WorkingDirectory $root -WindowStyle Hidden -PassThru -RedirectStandardOutput $log -RedirectStandardError $errLog
$processHandle = $process.Handle # Keep the exit code available after a short-lived GUI process exits.
if (!$process.WaitForExit(20000)) {
    Stop-Process -Id $process.Id
    throw "Window smoke test timed out"
}
if ($process.ExitCode -ne 0) {
    Get-Content $log, $errLog
    throw "Window smoke test failed: $($process.ExitCode)"
}
Write-Output "$Label : PASS (window created, 120 frames rendered, clean exit)"

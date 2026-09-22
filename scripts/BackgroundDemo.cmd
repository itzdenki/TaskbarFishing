@echo off
setlocal
set "demo_exe=%~dp0TaskbarFishing.exe"
if not exist "%demo_exe%" set "demo_exe=%~dp0..\build\TaskbarFishing.exe"
if not exist "%demo_exe%" (
    echo Build TaskbarFishing before opening this preview.
    pause
    exit /b 1
)
start "" "%demo_exe%" --background-demo

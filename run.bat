@echo off
setlocal

echo =====================================================================
echo    LAUNCHING INTELLIGENT DISASTER EVACUATION ROUTE PLANNER
echo =====================================================================

REM If binary is missing, automatically trigger build
if not exist "bin\evac_server.exe" (
    echo [INFO] Server binary not found. Triggering build.bat first...
    call build.bat
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Build step failed. Cannot launch server.
        pause
        exit /b 1
    )
)

echo [INFO] Starting Evacuation Backend Server on http://localhost:8080...
echo [INFO] Launching default web browser...

REM Open default browser after a brief delay
start "" http://localhost:8080

REM Run C++ backend in this console
bin\evac_server.exe 8080
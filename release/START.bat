@echo off
setlocal enabledelayedexpansion

title Emergency Route Planner - Server Console
cd /d "%~dp0"

echo =====================================================================
echo    INTELLIGENT DISASTER EVACUATION ROUTE PLANNER
echo    Standalone Windows Launcher
echo =====================================================================
echo.
echo [1/4] Checking application files in "%CD%"...

if not exist "bin\evac_server.exe" (
    echo.
    echo [ERROR] Required backend executable not found:
    echo         "%CD%\bin\evac_server.exe"
    echo.
    echo If you cloned the source repository, run build.bat first.
    echo If you downloaded the release ZIP, please extract all files cleanly.
    echo.
    pause
    exit /b 1
)

if not exist "data\city_data.json" (
    echo.
    echo [ERROR] Required city network data not found:
    echo         "%CD%\data\city_data.json"
    echo.
    echo Please ensure the "data" folder is in the same folder as START.bat.
    echo.
    pause
    exit /b 1
)

if not exist "frontend\index.html" (
    echo.
    echo [ERROR] Required web dashboard files not found:
    echo         "%CD%\frontend\index.html"
    echo.
    echo Please ensure the "frontend" folder is in the same folder as START.bat.
    echo.
    pause
    exit /b 1
)

echo [2/4] Checking port 8080 availability...
netstat -ano -p TCP 2>nul | findstr /R /C:":8080 .*LISTENING" >nul
if %ERRORLEVEL% equ 0 (
    echo.
    echo [ERROR] Port 8080 is already in use by another process.
    echo         Close any existing Emergency Route Planner console window
    echo         or stop the program using port 8080, then run START.bat again.
    echo.
    pause
    exit /b 1
)

echo [3/4] Preparing automatic browser launch for http://localhost:8080...
where powershell >nul 2>&1
if %ERRORLEVEL% equ 0 (
    start "" /b powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Sleep -Milliseconds 350; for ($i = 0; $i -lt 24; $i++) { try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8080/api/network' -UseBasicParsing -TimeoutSec 1 -ErrorAction Stop; if ($r.StatusCode -eq 200) { Start-Process 'http://localhost:8080'; break } } catch {}; if ($i -ge 6 -and -not (Get-Process -Name evac_server -ErrorAction SilentlyContinue)) { break }; Start-Sleep -Milliseconds 250 }" >nul 2>&1
) else (
    start "" /b cmd /c "ping -n 3 127.0.0.1 >nul & start "" http://localhost:8080" >nul 2>&1
)

echo [4/4] Starting C++ evacuation backend server on port 8080...
echo.
echo ---------------------------------------------------------------------
echo   Application URL : http://localhost:8080
echo   Keep this window OPEN while using the Evacuation Route Planner.
echo   To close safely : Press Ctrl+C here or close this console window.
echo ---------------------------------------------------------------------
echo.

set "T_START=%TIME: =0%"
for /f "tokens=1-4 delims=:.," %%a in ("!T_START!") do (
    set /a "START_CS=((1%%a-100)*360000 + (1%%b-100)*6000 + (1%%c-100)*100 + (1%%d-100))"
)

"bin\evac_server.exe" 8080
set "EXIT_CODE=%ERRORLEVEL%"

set "T_END=%TIME: =0%"
for /f "tokens=1-4 delims=:.," %%a in ("!T_END!") do (
    set /a "END_CS=((1%%a-100)*360000 + (1%%b-100)*6000 + (1%%c-100)*100 + (1%%d-100))"
)
set /a "ELAPSED_CS=END_CS - START_CS"
if !ELAPSED_CS! lss 0 set /a "ELAPSED_CS+=8640000"

set "IS_POLICY_CODE=0"
if "%EXIT_CODE%"=="1260" set "IS_POLICY_CODE=1"
if "%EXIT_CODE%"=="4551" set "IS_POLICY_CODE=1"
if "%EXIT_CODE%"=="5" set "IS_POLICY_CODE=1"
if "%EXIT_CODE%"=="-1073740760" set "IS_POLICY_CODE=1"
if "%EXIT_CODE%"=="-1073741790" set "IS_POLICY_CODE=1"

echo.
if "%EXIT_CODE%"=="0" (
    echo [INFO] Backend server stopped cleanly.
) else if "%EXIT_CODE%"=="-1073741510" (
    echo [INFO] Backend server stopped by user ^(Ctrl+C^).
) else if "%IS_POLICY_CODE%"=="1" (
    echo [BLOCKED] Windows prevented bin\evac_server.exe from running ^(exit code %EXIT_CODE%^).
    echo.
    echo WHY THIS HAPPENS:
    echo   Organization- or school-managed PCs using Windows Device Guard ^(WDAC^),
    echo   AppLocker, or Group Policy often block unsigned executables that have not
    echo   been explicitly approved by an IT administrator.
    echo.
    echo WHAT YOU CAN DO:
    echo   1. Check README.md for the SHA-256 checksum of bin\evac_server.exe and
    echo      request an allowlist approval from your IT administrator.
    echo   2. Or run this package on a personal, unmanaged Windows 10/11 computer.
) else if !ELAPSED_CS! lss 300 (
    echo [ERROR] bin\evac_server.exe terminated immediately on startup ^(exit code %EXIT_CODE%^).
    echo.
    echo POSSIBLE CAUSES:
    echo   - Blocked by Windows Device Guard ^(WDAC^), AppLocker, or SmartScreen on a
    echo     managed computer ^(the executable is currently unsigned^).
    echo   - Blocked or quarantined by antivirus / endpoint protection software.
    echo   - Failed to bind to port 8080 or failed to load data\city_data.json.
    echo.
    echo See README.md for IT administrator approval details and SHA-256 verification.
) else (
    echo [ERROR] Backend server stopped unexpectedly ^(exit code %EXIT_CODE%^).
    echo         Verify port 8080 is free and data\city_data.json is readable.
)
echo.
pause
exit /b %EXIT_CODE%

@echo off
setlocal enabledelayedexpansion

echo =====================================================================
echo    INTELLIGENT DISASTER EVACUATION ROUTE PLANNER - BUILD SYSTEM
echo    ADSA Project ^| C++14 Backend Compiler
echo =====================================================================

REM Ensure bin directory exists
if not exist "bin" mkdir bin

echo [1/3] Checking C++ compiler (g++)...
where g++ >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] g++ compiler not found in system PATH.
    echo Please install MinGW or add C:\MinGW\bin to your Windows PATH.
    pause
    exit /b 1
)

echo [2/3] Compiling ADSA Algorithm Validation Suite (tests/algorithm_tests.cpp)...
g++ -std=c++14 -O2 -static -static-libgcc -static-libstdc++ -Ibackend/include backend/Graph.cpp backend/Algorithms.cpp tests/algorithm_tests.cpp -o bin/algorithm_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Test suite compilation failed.
    pause
    exit /b 1
)
echo      - Successfully generated bin/algorithm_tests.exe

echo [3/3] Compiling Evacuation Backend Server (backend/main.cpp)...
g++ -std=c++14 -O2 -static -static-libgcc -static-libstdc++ -Ibackend/include backend/main.cpp backend/Graph.cpp backend/Algorithms.cpp backend/HttpServer.cpp backend/api.cpp -o bin/evac_server.exe -lws2_32
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Evacuation server compilation failed.
    pause
    exit /b 1
)
echo      - Successfully generated bin/evac_server.exe

echo.
echo =====================================================================
echo    BUILD SUCCESSFUL! RUNNING AUTOMATED ALGORITHM VERIFICATION...
echo =====================================================================
bin\algorithm_tests.exe
if %ERRORLEVEL% neq 0 (
    echo [WARNING] Some algorithm unit tests failed.
    pause
    exit /b 1
)

echo.
echo =====================================================================
echo  Ready to run! Execute run.bat to launch the server and web dashboard.
echo =====================================================================
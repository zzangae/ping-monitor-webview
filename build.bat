@echo off
chcp 65001 >nul
echo ============================================
echo   Ping Monitor WebView2 - Build Script
echo ============================================
echo.

REM MinGW-w64 컴파일
echo [1/2] Compiling with MinGW-w64...
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed!
    echo Make sure MinGW-w64 is installed and in PATH.
    pause
    exit /b 1
)

echo [OK] ping_monitor.exe created successfully!
echo.

REM 파일 확인
echo [2/2] Checking required files...

if exist "graph.html" (
    echo [OK] graph.html found
) else (
    echo [WARNING] graph.html not found - copy it to the same folder!
)

if exist "ping_config.ini" (
    echo [OK] ping_config.ini found
) else (
    echo [INFO] ping_config.ini not found - will use defaults
)

echo.
echo ============================================
echo   Build Complete!
echo ============================================
echo.
echo To run:
echo   1. Run ping_monitor.exe
echo   2. Browser opens automatically (http://localhost:8080)
echo   3. Right-click tray icon for options
echo.
echo Features:
echo   - Built-in HTTP server (port 8080)
echo   - System tray icon
echo   - Auto browser launch
echo.
pause
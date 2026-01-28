@echo off
echo ========================================
echo Ping Monitor v2.5 - Build
echo ========================================
echo.

echo [1/3] Compiling...
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo [2/3] Checking files...
if not exist ping_config.ini (
    echo WARNING: ping_config.ini not found
)
if not exist graph.html (
    echo WARNING: graph.html not found
)

echo [3/3] Build complete!
echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executable: ping_monitor.exe
echo Dashboard: http://localhost:8080/graph.html
echo.
echo New in v2.5:
echo   - Dual config files (ping_config.ini + int_config.ini)
echo   - Internal IPs protected (Git ignored)
echo.
echo Press any key to start ping_monitor.exe...
pause >nul

echo.
echo Starting ping_monitor.exe...
start ping_monitor.exe
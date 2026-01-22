@echo off
REM Ping Monitor with Notifications - Build Script
REM Windows Network Monitoring Tool with Notification Support

echo ========================================
echo Ping Monitor (v2.3 - Notifications)
echo ========================================
echo.

REM Check for compiler
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] gcc not found.
    echo Please install MinGW-w64 and add it to PATH.
    echo.
    echo Download: https://www.mingw-w64.org/
    pause
    exit /b 1
)

echo [1/3] Compiling...
gcc -o ping_monitor.exe ^
    ping_monitor_webview.c ^
    http_server.c ^
    -lws2_32 ^
    -liphlpapi ^
    -lshlwapi ^
    -lole32 ^
    -loleaut32 ^
    -lshell32 ^
    -mwindows ^
    -municode ^
    -O2

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo [2/3] Checking configuration file...
if not exist ping_config.ini (
    echo [WARNING] ping_config.ini not found.
    echo Please create a config file.
)

echo [3/3] Checking HTML file...
if not exist graph.html (
    echo [WARNING] graph.html not found.
    echo The program requires graph.html to work properly.
)

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executable: ping_monitor.exe
echo Config file: ping_config.ini
echo Web Dashboard: http://localhost:8080/graph.html
echo.
echo New Features (v2.3):
echo   - Tray balloon notifications on timeout
echo   - Connection recovery notifications
echo   - Toggle notifications from tray menu
echo   - Configurable via ping_config.ini [Settings]
echo.
echo Usage:
echo   ping_monitor.exe
echo.
pause
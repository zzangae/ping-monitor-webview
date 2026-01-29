@echo off
echo ========================================
echo Ping Monitor v2.5 - Build
echo ========================================
echo.

echo [1/4] Compiling...
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo [2/4] Checking files...
set MISSING_FILES=0

if not exist ping_config.ini (
    echo WARNING: ping_config.ini not found
    set MISSING_FILES=1
)
if not exist graph.html (
    echo WARNING: graph.html not found
    set MISSING_FILES=1
)
if not exist chart.umd.min.js (
    echo WARNING: chart.umd.min.js not found
    set MISSING_FILES=1
)
if not exist css\variables.css (
    echo WARNING: css\variables.css not found
    set MISSING_FILES=1
)
if not exist css\base.css (
    echo WARNING: css\base.css not found
    set MISSING_FILES=1
)
if not exist css\components.css (
    echo WARNING: css\components.css not found
    set MISSING_FILES=1
)
if not exist css\dashboard.css (
    echo WARNING: css\dashboard.css not found
    set MISSING_FILES=1
)
if not exist css\notifications.css (
    echo WARNING: css\notifications.css not found
    set MISSING_FILES=1
)
if not exist css\responsive.css (
    echo WARNING: css\responsive.css not found
    set MISSING_FILES=1
)

echo [3/4] Build complete!
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
echo   - Port change via tray icon
echo   - Browser close = program exit
echo.

if %MISSING_FILES% EQU 1 (
    echo WARNING: Some required files are missing!
    echo The program may not work correctly.
    echo.
)

echo [4/4] Choose action:
echo   1. Run program
echo   2. Create deployment package
echo   3. Exit
echo.
choice /c 123 /n /m "Select [1-3]: "

if errorlevel 3 goto :end
if errorlevel 2 goto :deploy
if errorlevel 1 goto :run

:run
echo.
echo Starting ping_monitor.exe...
start ping_monitor.exe
goto :end

:deploy
echo.
echo ========================================
echo Creating Deployment Package
echo ========================================
echo.

set DEPLOY_DIR=PingMonitor

if exist %DEPLOY_DIR% (
    echo Removing old deployment folder...
    rmdir /s /q %DEPLOY_DIR%
)

echo Creating deployment structure...
mkdir %DEPLOY_DIR%
mkdir %DEPLOY_DIR%\css

echo Copying files...
copy ping_monitor.exe %DEPLOY_DIR%\ >nul
copy graph.html %DEPLOY_DIR%\ >nul

if exist chart.umd.min.js (
    copy chart.umd.min.js %DEPLOY_DIR%\ >nul
    echo   - chart.umd.min.js copied
) else (
    echo WARNING: chart.umd.min.js not found!
    echo   Download from: https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
)

if exist ping_config.ini (
    copy ping_config.ini %DEPLOY_DIR%\ >nul
) else (
    echo Creating sample ping_config.ini...
    (
        echo [Settings]
        echo NotificationsEnabled=1
        echo NotificationCooldown=60
        echo NotifyOnTimeout=1
        echo NotifyOnRecovery=1
        echo ConsecutiveFailures=3
        echo.
        echo 8.8.8.8, Google DNS
        echo 1.1.1.1, Cloudflare DNS
    ) > %DEPLOY_DIR%\ping_config.ini
)

if exist int_config.ini (
    copy int_config.ini %DEPLOY_DIR%\ >nul
)

copy css\*.css %DEPLOY_DIR%\css\ >nul

echo Creating README...
(
    echo ====================================================
    echo Ping Monitor WebView2 v2.5
    echo ====================================================
    echo.
    echo HOW TO RUN:
    echo   Double-click ping_monitor.exe
    echo.
    echo DEFAULT PORT:
    echo   http://localhost:8080
    echo.
    echo CONFIG FILES:
    echo   ping_config.ini - Public IPs
    echo   int_config.ini  - Internal IPs (optional^)
    echo.
    echo TROUBLESHOOTING:
    echo   404 Error: Check all files are in correct location
    echo   Port Error: Right-click tray icon ^> Change Port
    echo   Chart Error: Make sure chart.umd.min.js exists
    echo.
    echo REQUIRED FILES:
    echo   ping_monitor.exe
    echo   graph.html
    echo   chart.umd.min.js
    echo   ping_config.ini
    echo   css\ folder (6 CSS files^)
    echo.
    echo FEATURES:
    echo   - Real-time ICMP ping monitoring
    echo   - Interactive web dashboard with Chart.js
    echo   - Network timeout/recovery notifications
    echo   - IP card drag and drop
    echo   - Minimize/restore IP cards
    echo   - Port configuration via tray icon
    echo   - Browser close = program exit
    echo.
    echo GitHub:
    echo   https://github.com/zzangae/ping-monitor-webview
    echo.
) > %DEPLOY_DIR%\README.txt

echo.
echo ========================================
echo Deployment Package Created!
echo ========================================
echo.
echo Location: %CD%\%DEPLOY_DIR%
echo.
echo Files included:
dir /b %DEPLOY_DIR%
echo.
echo CSS files:
dir /b %DEPLOY_DIR%\css
echo.
echo You can now copy the '%DEPLOY_DIR%' folder to other PCs.
echo.
pause
goto :end

:end
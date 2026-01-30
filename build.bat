@echo off
chcp 65001 > nul
echo ========================================
echo Ping Monitor v2.6 - Build
echo ========================================
echo.

echo [0/5] Diagnostic Info...
echo Current Directory: %CD%
echo.
dir /b ping_monitor_webview.c http_server.c graph.html 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Source files not found in current directory!
    echo Please run build.bat from the project folder.
    pause
    exit /b 1
)
echo ✓ Source files found
echo.

echo [1/5] Checking running processes...
tasklist /FI "IMAGENAME eq ping_monitor.exe" 2>NUL | find /I /N "ping_monitor.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo Found running ping_monitor.exe - terminating...
    taskkill /F /IM ping_monitor.exe >NUL 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo ✓ Process terminated successfully
        timeout /t 1 /nobreak >NUL
    ) else (
        echo WARNING: Failed to terminate process
    )
) else (
    echo ✓ No running process found
)
echo.

echo [2/5] Compiling...
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c outage.c config_api.c browser_monitor.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)
echo ✓ Compilation successful
echo.

echo [3/5] Checking required files...
echo.
echo Checking in: %CD%
echo.

set MISSING_FILES=0

if exist ping_config.ini (
    echo ✓ ping_config.ini
) else (
    echo ✗ ping_config.ini NOT FOUND
    set MISSING_FILES=1
)

if exist graph.html (
    echo ✓ graph.html
) else (
    echo ✗ graph.html NOT FOUND
    set MISSING_FILES=1
)

if exist chart.umd.min.js (
    echo ✓ chart.umd.min.js
) else (
    echo ⚠ chart.umd.min.js (optional - CDN fallback available)
)

echo.
echo CSS Files:
if exist css\variables.css (
    echo ✓ css\variables.css
) else (
    echo ✗ css\variables.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\base.css (
    echo ✓ css\base.css
) else (
    echo ✗ css\base.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\components.css (
    echo ✓ css\components.css
) else (
    echo ✗ css\components.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\dashboard.css (
    echo ✓ css\dashboard.css
) else (
    echo ✗ css\dashboard.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\notifications.css (
    echo ✓ css\notifications.css
) else (
    echo ✗ css\notifications.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\outages.css (
    echo ✓ css\outages.css
) else (
    echo ✗ css\outages.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\settings.css (
    echo ✓ css\settings.css
) else (
    echo ✗ css\settings.css NOT FOUND
    set MISSING_FILES=1
)

if exist css\responsive.css (
    echo ✓ css\responsive.css
) else (
    echo ✗ css\responsive.css NOT FOUND
    set MISSING_FILES=1
)

echo.

if %MISSING_FILES% EQU 1 (
    echo ========================================
    echo ERROR: Required files are missing!
    echo ========================================
    echo.
    echo The program will not work correctly.
    echo Please ensure all files are in the same folder as build.bat
    echo.
    pause
    exit /b 1
)

echo [4/5] Build complete!
echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executable: ping_monitor.exe
echo Dashboard: http://localhost:8080/graph.html
echo All files: OK
echo.
echo New in v2.6:
echo   - Outage Management System (5min threshold)
echo   - Group and Priority support
echo   - Outage Timeline Tab
echo   - Integrated Settings UI (⚙️ button)
echo   - outage_log.json recording
echo.

echo [5/5] Choose action:
echo   1. Run program
echo   2. Create deployment package
echo   3. Run with console (debug mode)
echo   4. Exit
echo.
choice /c 1234 /n /m "Select [1-4]: "

if errorlevel 4 goto :end
if errorlevel 3 goto :debug
if errorlevel 2 goto :deploy
if errorlevel 1 goto :run

:run
echo.
echo Starting ping_monitor.exe...
start ping_monitor.exe
goto :end

:debug
echo.
echo ========================================
echo Debug Mode
echo ========================================
echo.
echo Recompiling with console output...
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c outage.c config_api.c browser_monitor.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -lgdi32 -municode -O0 -g

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo.
echo Starting in debug mode...
echo Watch console for error messages!
echo.
ping_monitor.exe
echo.
echo Program terminated.
pause
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
    echo   ✓ chart.umd.min.js copied
) else (
    echo   ⚠ chart.umd.min.js not found - using CDN fallback
    echo     Download from: https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
)

if exist ping_config.ini (
    copy ping_config.ini %DEPLOY_DIR%\ >nul
) else (
    echo Creating sample ping_config.ini...
    (
        echo [OutageDetection]
        echo OutageThreshold=300
        echo ServerOutageThreshold=180
        echo NetworkOutageThreshold=300
        echo FirewallOutageThreshold=120
        echo.
        echo [IPGroups]
        echo 8.8.8.8=네트워크,1
        echo 1.1.1.1=네트워크,1
        echo.
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
    echo Ping Monitor WebView2 v2.6
    echo ====================================================
    echo.
    echo HOW TO RUN:
    echo   Double-click ping_monitor.exe
    echo.
    echo DEFAULT PORT:
    echo   http://localhost:8080
    echo.
    echo CONFIG FILES:
    echo   ping_config.ini - Public IPs + Outage Settings
    echo   int_config.ini  - Internal IPs (optional^)
    echo.
    echo NEW IN v2.6:
    echo   - Outage Management System (5min threshold^)
    echo   - Group classification (7 groups^)
    echo   - Priority levels (P1-P5^)
    echo   - Integrated Settings UI (⚙️ button^)
    echo   - Outage Timeline Tab
    echo   - outage_log.json logging
    echo.
    echo FEATURES:
    echo   - Real-time ICMP ping monitoring
    echo   - Interactive web dashboard with Chart.js
    echo   - Network timeout/recovery notifications
    echo   - IP card drag and drop
    echo   - Minimize/restore IP cards
    echo   - Port configuration via tray icon
    echo   - Browser close = program exit
    echo   - Settings UI for group thresholds and IP management
    echo.
    echo REQUIRED FILES:
    echo   - ping_monitor.exe
    echo   - graph.html
    echo   - chart.umd.min.js (optional, CDN fallback available^)
    echo   - ping_config.ini
    echo   - css\ folder (8 CSS files^)
    echo.
    echo CHART.JS LIBRARY:
    echo   If chart.umd.min.js is missing, the dashboard will use CDN.
    echo   For offline use, download from:
    echo   https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
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
@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================
echo   Ping Monitor v2.7 Build
echo ============================================
echo.

:menu
echo Select menu:
echo   1. Compile
echo   2. Debug mode (console output + log file)
echo   3. Run
echo   4. Create release package
echo   5. Clear screen
echo   6. Exit
echo.
set /p choice="Select (1-6): "

if "%choice%"=="1" goto compile_only
if "%choice%"=="2" goto debug_mode
if "%choice%"=="3" goto run_only
if "%choice%"=="4" goto create_release
if "%choice%"=="5" goto clear_screen
if "%choice%"=="6" goto end
echo Invalid selection.
goto menu

:compile_only
call :do_compile
goto menu

:debug_mode
echo.
echo Compiling in debug mode...
cd main
gcc -c ping_monitor_webview.c -o ping_monitor_webview.o -municode -DDEBUG
gcc -c module\config.c -o module_config.o -DDEBUG
gcc -c module\network.c -o module_network.o -DDEBUG
gcc -c module\notification.c -o module_notification.o -DDEBUG
gcc -c module\port.c -o module_port.o -DDEBUG
gcc -c outage.c -o outage.o -DDEBUG

gcc -o ping_monitor.exe ^
    ping_monitor_webview.o ^
    module_config.o ^
    module_network.o ^
    module_notification.o ^
    module_port.o ^
    outage.o ^
    module\tray.c ^
    http_server.c ^
    browser_monitor.c ^
    config_api.c ^
    -municode ^
    -lws2_32 -liphlpapi -lshlwapi -lshell32 -lole32 -loleaut32 -luuid -lgdi32

if %errorlevel% neq 0 (
    echo Compile failed!
    cd ..
    goto menu
)

move ping_monitor.exe .. >nul
del *.o >nul 2>&1
cd ..

echo Debug mode compile complete!
echo Console window will be displayed with log output.
start "" ping_monitor.exe
goto menu

:run_only
echo.
if exist ping_monitor.exe (
    echo Starting program...
    start "" ping_monitor.exe
) else (
    echo [Error] ping_monitor.exe not found. Please compile first.
)
echo.
goto menu

:clear_screen
cls
echo ============================================
echo   Ping Monitor v2.7 Build
echo ============================================
echo.
goto menu

:do_compile
echo.
echo Compiling...
cd main

gcc -c ping_monitor_webview.c -o ping_monitor_webview.o -municode -mwindows
if %errorlevel% neq 0 (
    echo ping_monitor_webview.c compile failed!
    cd ..
    exit /b 1
)

gcc -c module\config.c -o module_config.o
if %errorlevel% neq 0 (
    echo config.c compile failed!
    cd ..
    exit /b 1
)

gcc -c module\network.c -o module_network.o
if %errorlevel% neq 0 (
    echo network.c compile failed!
    cd ..
    exit /b 1
)

gcc -c module\notification.c -o module_notification.o
if %errorlevel% neq 0 (
    echo notification.c compile failed!
    cd ..
    exit /b 1
)

gcc -c module\port.c -o module_port.o
if %errorlevel% neq 0 (
    echo port.c compile failed!
    cd ..
    exit /b 1
)

gcc -c outage.c -o outage.o
if %errorlevel% neq 0 (
    echo outage.c compile failed!
    cd ..
    exit /b 1
)

echo Linking...
gcc -o ping_monitor.exe ^
    ping_monitor_webview.o ^
    module_config.o ^
    module_network.o ^
    module_notification.o ^
    module_port.o ^
    outage.o ^
    module\tray.c ^
    http_server.c ^
    browser_monitor.c ^
    config_api.c ^
    -municode -mwindows ^
    -lws2_32 -liphlpapi -lshlwapi -lshell32 -lole32 -loleaut32 -luuid -lgdi32

if %errorlevel% neq 0 (
    echo Linking failed!
    cd ..
    exit /b 1
)

move ping_monitor.exe .. >nul
del *.o >nul 2>&1
cd ..

echo.
echo ============================================
echo   Compile successful!
echo ============================================
exit /b 0

:create_release
echo.
echo ============================================
echo   Create Release Package v2.7
echo ============================================
echo.

set RELEASE_DIR=PingMonitor_v2.7_Release

:: Delete existing folder and zip file
if exist %RELEASE_DIR% (
    echo Deleting existing release folder...
    rmdir /s /q %RELEASE_DIR%
)
if exist %RELEASE_DIR%.zip (
    echo Deleting existing zip file...
    del /f /q %RELEASE_DIR%.zip
)

:: Create folder structure
echo Creating folder structure...
mkdir %RELEASE_DIR%
mkdir %RELEASE_DIR%\config
mkdir %RELEASE_DIR%\data
mkdir %RELEASE_DIR%\web
mkdir %RELEASE_DIR%\web\css
mkdir %RELEASE_DIR%\web\config
mkdir %RELEASE_DIR%\web\section-ip
mkdir %RELEASE_DIR%\web\minwin-ip

:: Copy executable
echo Copying executable...
if exist ping_monitor.exe (
    copy ping_monitor.exe %RELEASE_DIR%\ >nul
) else (
    echo [Warning] ping_monitor.exe not found. Please compile first.
)

:: Copy config files
echo Copying config files...
if exist config\ping_config.ini copy config\ping_config.ini %RELEASE_DIR%\config\ >nul
if exist config\int_config.ini copy config\int_config.ini %RELEASE_DIR%\config\ >nul

:: Create .gitkeep in data folder
echo. > %RELEASE_DIR%\data\.gitkeep

:: Copy web files
echo Copying web files...
if exist web\graph.html copy web\graph.html %RELEASE_DIR%\web\ >nul
if exist web\chart.umd.min.js copy web\chart.umd.min.js %RELEASE_DIR%\web\ >nul

:: Copy CSS files
echo Copying CSS files...
if exist web\css\variables.css copy web\css\variables.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\base.css copy web\css\base.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\components.css copy web\css\components.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\dashboard.css copy web\css\dashboard.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\notifications.css copy web\css\notifications.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\outages.css copy web\css\outages.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\settings.css copy web\css\settings.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\responsive.css copy web\css\responsive.css %RELEASE_DIR%\web\css\ >nul

:: Copy web/config files
echo Copying web/config files...
if exist web\config\settings.css copy web\config\settings.css %RELEASE_DIR%\web\config\ >nul
if exist web\config\settings.js copy web\config\settings.js %RELEASE_DIR%\web\config\ >nul

:: Copy web/section-ip files
echo Copying web/section-ip files...
if exist web\section-ip\timeline.css copy web\section-ip\timeline.css %RELEASE_DIR%\web\section-ip\ >nul
if exist web\section-ip\timeline.js copy web\section-ip\timeline.js %RELEASE_DIR%\web\section-ip\ >nul
if exist web\section-ip\timeline.html copy web\section-ip\timeline.html %RELEASE_DIR%\web\section-ip\ >nul

:: Copy web/minwin-ip files
echo Copying web/minwin-ip files...
if exist web\minwin-ip\minwin-ip.css copy web\minwin-ip\minwin-ip.css %RELEASE_DIR%\web\minwin-ip\ >nul
if exist web\minwin-ip\minwin-ip.js copy web\minwin-ip\minwin-ip.js %RELEASE_DIR%\web\minwin-ip\ >nul
if exist web\minwin-ip\minwin-ip.html copy web\minwin-ip\minwin-ip.html %RELEASE_DIR%\web\minwin-ip\ >nul

:: Create zip file (using PowerShell)
echo.
echo Creating zip file...
powershell -command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%RELEASE_DIR%.zip' -Force"

echo.
echo ============================================
echo   Release package created!
echo ============================================
echo.
echo Generated files:
echo   - %RELEASE_DIR%\           (folder)
echo   - %RELEASE_DIR%.zip        (zip file)
echo.
goto menu

:end
echo.
echo Exiting program.
exit /b 0
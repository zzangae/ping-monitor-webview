@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================
echo   Ping 응답 확인 시각화 도구 v2.7 빌드
echo ============================================
echo.

:menu
echo 메뉴를 선택하세요:
echo   1. 컴파일 및 실행
echo   2. 컴파일만
echo   3. 디버그 모드 (콘솔 출력 + 로그 파일)
echo   4. 배포 패키지 생성
echo   5. 종료
echo.
set /p choice="선택 (1-5): "

if "%choice%"=="1" goto compile_run
if "%choice%"=="2" goto compile_only
if "%choice%"=="3" goto debug_mode
if "%choice%"=="4" goto create_release
if "%choice%"=="5" goto end
echo 잘못된 선택입니다.
goto menu

:compile_run
call :do_compile
if %errorlevel% neq 0 goto menu
echo.
echo 프로그램을 실행합니다...
start "" ping_monitor.exe
goto menu

:compile_only
call :do_compile
goto menu

:debug_mode
echo.
echo 디버그 모드로 컴파일 중...
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
    echo 컴파일 실패!
    cd ..
    goto menu
)

move ping_monitor.exe .. >nul
del *.o >nul 2>&1
cd ..

echo 디버그 모드 컴파일 완료!
echo 콘솔 창이 표시되며 로그가 출력됩니다.
start "" ping_monitor.exe
goto menu

:do_compile
echo.
echo 컴파일 중...
cd main

gcc -c ping_monitor_webview.c -o ping_monitor_webview.o -municode -mwindows
if %errorlevel% neq 0 (
    echo ping_monitor_webview.c 컴파일 실패!
    cd ..
    exit /b 1
)

gcc -c module\config.c -o module_config.o
if %errorlevel% neq 0 (
    echo config.c 컴파일 실패!
    cd ..
    exit /b 1
)

gcc -c module\network.c -o module_network.o
if %errorlevel% neq 0 (
    echo network.c 컴파일 실패!
    cd ..
    exit /b 1
)

gcc -c module\notification.c -o module_notification.o
if %errorlevel% neq 0 (
    echo notification.c 컴파일 실패!
    cd ..
    exit /b 1
)

gcc -c module\port.c -o module_port.o
if %errorlevel% neq 0 (
    echo port.c 컴파일 실패!
    cd ..
    exit /b 1
)

gcc -c outage.c -o outage.o
if %errorlevel% neq 0 (
    echo outage.c 컴파일 실패!
    cd ..
    exit /b 1
)

echo 링킹 중...
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
    echo 링킹 실패!
    cd ..
    exit /b 1
)

move ping_monitor.exe .. >nul
del *.o >nul 2>&1
cd ..

echo.
echo ============================================
echo   컴파일 성공!
echo ============================================
exit /b 0

:create_release
echo.
echo ============================================
echo   배포 패키지 생성 v2.7
echo ============================================
echo.

set RELEASE_DIR=PingMonitor_v2.7_Release

:: 기존 폴더 및 압축 파일 삭제
if exist %RELEASE_DIR% (
    echo 기존 배포 폴더 삭제 중...
    rmdir /s /q %RELEASE_DIR%
)
if exist %RELEASE_DIR%.zip (
    echo 기존 압축 파일 삭제 중...
    del /f /q %RELEASE_DIR%.zip
)

:: 폴더 구조 생성
echo 폴더 구조 생성 중...
mkdir %RELEASE_DIR%
mkdir %RELEASE_DIR%\config
mkdir %RELEASE_DIR%\data
mkdir %RELEASE_DIR%\web
mkdir %RELEASE_DIR%\web\css
mkdir %RELEASE_DIR%\web\config
mkdir %RELEASE_DIR%\web\section-ip

:: 실행 파일 복사
echo 실행 파일 복사 중...
if exist ping_monitor.exe (
    copy ping_monitor.exe %RELEASE_DIR%\ >nul
) else (
    echo [경고] ping_monitor.exe가 없습니다. 먼저 컴파일하세요.
)

:: 설정 파일 복사
echo 설정 파일 복사 중...
if exist config\ping_config.ini copy config\ping_config.ini %RELEASE_DIR%\config\ >nul
if exist config\int_config.ini copy config\int_config.ini %RELEASE_DIR%\config\ >nul

:: data 폴더에 .gitkeep 생성
echo. > %RELEASE_DIR%\data\.gitkeep

:: 웹 파일 복사
echo 웹 파일 복사 중...
if exist web\graph.html copy web\graph.html %RELEASE_DIR%\web\ >nul
if exist web\chart.umd.min.js copy web\chart.umd.min.js %RELEASE_DIR%\web\ >nul

:: CSS 파일 복사
echo CSS 파일 복사 중...
if exist web\css\variables.css copy web\css\variables.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\base.css copy web\css\base.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\components.css copy web\css\components.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\dashboard.css copy web\css\dashboard.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\notifications.css copy web\css\notifications.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\outages.css copy web\css\outages.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\settings.css copy web\css\settings.css %RELEASE_DIR%\web\css\ >nul
if exist web\css\responsive.css copy web\css\responsive.css %RELEASE_DIR%\web\css\ >nul

:: web/config 파일 복사 (v2.7 신규)
echo web/config 파일 복사 중...
if exist web\config\settings.css copy web\config\settings.css %RELEASE_DIR%\web\config\ >nul
if exist web\config\settings.js copy web\config\settings.js %RELEASE_DIR%\web\config\ >nul

:: web/section-ip 파일 복사 (v2.7 신규)
echo web/section-ip 파일 복사 중...
if exist web\section-ip\timeline.css copy web\section-ip\timeline.css %RELEASE_DIR%\web\section-ip\ >nul
if exist web\section-ip\timeline.js copy web\section-ip\timeline.js %RELEASE_DIR%\web\section-ip\ >nul
if exist web\section-ip\timeline.html copy web\section-ip\timeline.html %RELEASE_DIR%\web\section-ip\ >nul

:: 압축 파일 생성 (PowerShell 사용)
echo.
echo 압축 파일 생성 중...
powershell -command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%RELEASE_DIR%.zip' -Force"

echo.
echo ============================================
echo   배포 패키지 생성 완료!
echo ============================================
echo.
echo 생성된 파일:
echo   - %RELEASE_DIR%\           (폴더)
echo   - %RELEASE_DIR%.zip        (압축 파일)
echo.
goto menu

:end
echo.
echo 프로그램을 종료합니다.
exit /b 0
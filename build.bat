@echo off
chcp 65001 > nul
cd /d "%~dp0"

echo ========================================
echo Ping Monitor v2.6 빌드 시스템
echo ========================================
echo.
echo 1. 컴파일 및 실행
echo 2. 컴파일만
echo 3. 디버그 모드
echo 4. 종료
echo.
set /p choice="선택: "

if "%choice%"=="1" goto compile_run
if "%choice%"=="2" goto compile_only
if "%choice%"=="3" goto debug_mode
if "%choice%"=="4" exit
goto end

:compile_only
echo.
echo [현재 디렉토리: %cd%]
echo.

echo [기존 exe 삭제]
if exist ping_monitor.exe (
    del ping_monitor.exe
    echo 기존 exe 삭제 완료
)
if exist main\ping_monitor.exe (
    del main\ping_monitor.exe
    echo main 폴더의 기존 exe 삭제 완료
)
echo.

echo [컴파일 시작]
cd main

echo [1/6] ping_monitor_webview.c
gcc -c ping_monitor_webview.c -o ping_monitor_webview.o -municode -mwindows
if errorlevel 1 (
    echo [오류] ping_monitor_webview.c 컴파일 실패
    goto error
)

echo [2/6] module\config.c
gcc -c module\config.c -o module_config.o
if errorlevel 1 (
    echo [오류] module\config.c 컴파일 실패
    goto error
)

echo [3/6] module\network.c
gcc -c module\network.c -o module_network.o
if errorlevel 1 (
    echo [오류] module\network.c 컴파일 실패
    goto error
)

echo [4/6] module\notification.c
gcc -c module\notification.c -o module_notification.o
if errorlevel 1 (
    echo [오류] module\notification.c 컴파일 실패
    goto error
)

echo [5/6] module\port.c
gcc -c module\port.c -o module_port.o
if errorlevel 1 (
    echo [오류] module\port.c 컴파일 실패
    goto error
)

echo [6/6] outage.c
gcc -c outage.c -o outage.o
if errorlevel 1 (
    echo [오류] outage.c 컴파일 실패
    goto error
)

echo.
echo [링킹 시작]
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
    -lws2_32 -liphlpapi -lshlwapi -lshell32 -lole32 -loleaut32 -luuid

if errorlevel 1 (
    echo [오류] 링킹 실패
    cd ..
    goto error
)

echo [청소]
del *.o 2>nul

echo.
echo [exe를 프로젝트 루트로 이동]
if exist ping_monitor.exe (
    move ping_monitor.exe ..
    cd ..
    if exist ping_monitor.exe (
        echo [성공] ping_monitor.exe 생성 완료
        echo 위치: %cd%\ping_monitor.exe
        dir ping_monitor.exe | findstr "ping_monitor.exe"
    ) else (
        echo [실패] exe 이동 실패
        goto error
    )
) else (
    echo [실패] exe 생성 안됨
    cd ..
    goto error
)

goto end

:compile_run
call :compile_only
if exist ping_monitor.exe (
    echo.
    echo [실행]
    start ping_monitor.exe
)
goto end

:debug_mode
echo.
echo [디버그 모드 컴파일]
cd main

if exist ping_monitor.exe (
    del ping_monitor.exe
)

gcc -g ^
    ping_monitor_webview.c ^
    module\config.c ^
    module\network.c ^
    module\notification.c ^
    module\port.c ^
    module\tray.c ^
    outage.c ^
    http_server.c ^
    browser_monitor.c ^
    config_api.c ^
    -o ping_monitor.exe -municode -mwindows ^
    -lws2_32 -liphlpapi -lshlwapi -lshell32 -lole32 -loleaut32 -luuid

if errorlevel 1 (
    echo [오류] 디버그 컴파일 실패
    cd ..
    goto error
)

if exist ping_monitor.exe (
    move ping_monitor.exe ..
    cd ..
    if exist ping_monitor.exe (
        echo [성공] 디버그 빌드 완료
        echo 위치: %cd%\ping_monitor.exe
        dir ping_monitor.exe | findstr "ping_monitor.exe"
    ) else (
        echo [실패] exe 이동 실패
        goto error
    )
) else (
    echo [실패] exe 생성 안됨
    cd ..
    goto error
)

goto end

:error
echo.
echo [오류] 빌드 실패
pause
exit /b 1

:end
echo.
pause
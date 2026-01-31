@echo off
chcp 65001 > nul
cd /d "%~dp0"

echo ========================================
echo 컴파일 진단 스크립트
echo ========================================
echo.

echo [1] 필수 파일 확인
echo.

set "missing=0"

if not exist "main\ping_monitor_webview.c" (
    echo [X] main\ping_monitor_webview.c 없음
    set "missing=1"
) else (
    echo [O] main\ping_monitor_webview.c
)

if not exist "main\module\config.c" (
    echo [X] main\module\config.c 없음
    set "missing=1"
) else (
    echo [O] main\module\config.c
)

if not exist "main\module\network.c" (
    echo [X] main\module\network.c 없음
    set "missing=1"
) else (
    echo [O] main\module\network.c
)

if not exist "main\module\notification.c" (
    echo [X] main\module\notification.c 없음
    set "missing=1"
) else (
    echo [O] main\module\notification.c
)

if not exist "main\module\port.c" (
    echo [X] main\module\port.c 없음
    set "missing=1"
) else (
    echo [O] main\module\port.c
)

if not exist "main\outage.c" (
    echo [X] main\outage.c 없음
    set "missing=1"
) else (
    echo [O] main\outage.c
)

echo.

if "%missing%"=="1" (
    echo [오류] 필수 파일이 누락되었습니다.
    pause
    exit /b 1
)

echo [2] 컴파일 테스트
echo.

cd main

gcc -c ping_monitor_webview.c -o test_main.o -municode -mwindows 2> compile_errors.txt
if errorlevel 1 (
    echo [X] ping_monitor_webview.c 컴파일 실패
    type compile_errors.txt
    del test_main.o 2>nul
    pause
    exit /b 1
) else (
    echo [O] ping_monitor_webview.c 컴파일 성공
    del test_main.o
)

gcc -c module\config.c -o test_config.o 2> compile_errors.txt
if errorlevel 1 (
    echo [X] module\config.c 컴파일 실패
    type compile_errors.txt
    del test_config.o 2>nul
    pause
    exit /b 1
) else (
    echo [O] module\config.c 컴파일 성공
    del test_config.o
)

gcc -c module\network.c -o test_network.o 2> compile_errors.txt
if errorlevel 1 (
    echo [X] module\network.c 컴파일 실패
    type compile_errors.txt
    del test_network.o 2>nul
    pause
    exit /b 1
) else (
    echo [O] module\network.c 컴파일 성공
    del test_network.o
)

echo.
echo [성공] 모든 파일이 정상입니다.
echo.
pause
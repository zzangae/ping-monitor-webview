# 4. INSTALLATION.md

[메인으로 돌아가기](../README.md)

## 문서

- [시스템 아키텍처](ARCHITECTURE.md)
- [파일 구조 및 배포](FILE_STRUCTURE.md)
- [설치 및 빌드 가이드](INSTALLATION.md)
- [사용자 가이드](USER_GUIDE.md)
- [설정 파일 상세](CONFIGURATION.md)
- [문제 해결](TROUBLESHOOTING.md)
- [버전 변경 이력](CHANGELOG.md)

---

## 📋 포함 내용

- 시스템 요구사항
- MinGW 설치 방법
- 빌드 방법 (build.bat)
- 수동 컴파일
- 배포 패키지 생성
- 다른 PC에 설치

---

## 💻 시스템 요구사항

### 개발 환경

- **OS**: Windows 10/11 (64-bit)
- **컴파일러**: MinGW-w64 GCC 14.2.0 이상
- **RAM**: 2GB 이상
- **디스크**: 100MB 이상

### 런타임 요구사항

- **OS**: Windows 10/11
- **브라우저**: Chrome, Edge, Firefox (최신 버전)
- **권한**: 관리자 권한 (ICMP 핑 사용 시 권장)
- **포트**: 8080-8099 중 1개 이상 사용 가능

---

## 🔧 MinGW-w64 설치

### 1. 다운로드

```
https://github.com/niXman/mingw-builds-binaries/releases

파일 선택:
x86_64-14.2.0-release-posix-seh-ucrt-rt_v12-rev0.7z
```

```bash
# Windows에서 MSYS2 사용
pacman -S mingw-w64-x86_64-gcc
```

### 2. 압축 해제

```batch
# 권장 경로
C:\mingw64\

# 압축 해제 후 폴더 구조
C:\mingw64\
├── bin\          # gcc.exe, g++.exe 등
├── include\
├── lib\
└── ...
```

**저장소 클론**

```bash
git clone https://github.com/zzangae/ping-monitor-webview.git
cd ping-monitor-webview
```

### 3. 환경 변수 설정

**시스템 속성 → 고급 → 환경 변수**

```
변수 이름: Path
값 추가: C:\mingw64\bin
```

### 4. 확인

```batch
# 새 CMD 창 열기
gcc --version

# 출력 예시:
# gcc (x86_64-posix-seh-rev0, Built by MinGW-W64 project) 14.2.0
```

---

## 🛠️ 빌드 방법

### build.bat 사용 (권장)

```batch
cd ping_monitor_v26
build.bat
```

### 수동 빌드 (선택)

```bash
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2
```

**옵션 선택:**

1. **Run program** - 즉시 실행
2. **Create deployment package** - 배포 패키지 생성
3. **Exit** - 종료


**메뉴 선택:**

```
========================================
Ping Monitor v2.6 빌드 시스템
========================================

1. 컴파일 및 실행
2. 컴파일만
3. 디버그 모드 (콘솔 출력 + 로그 파일)
4. 배포 패키지 생성
5. 종료

선택:
```

### 옵션 1: 컴파일 및 실행

```batch
# 선택: 1
# 동작:
#   1. 기존 exe 삭제
#   2. 소스 컴파일
#   3. 링킹
#   4. ping_monitor.exe 생성
#   5. 자동 실행
```

### 옵션 2: 컴파일만

```batch
# 선택: 2
# 동작:
#   1. 기존 exe 삭제
#   2. 소스 컴파일
#   3. 링킹
#   4. ping_monitor.exe 생성
#   (자동 실행 안 함)
```

### 옵션 3: 디버그 모드

```batch
# 선택: 3
# 동작:
#   1. -mconsole 플래그로 컴파일
#   2. 콘솔 창에 모든 출력 표시
#   3. ping_monitor_debug.log 파일 생성
#   4. 오류 진단 용이
```

**디버그 로그 예시:**

```
HTTP 서버 시작 시도 (포트: 8080)...
HTTP 서버 시작 성공: http://localhost:8080
설정 로드 완료: 총 5개 타겟
Browser launched successfully
알림: 활성화
```

### 옵션 4: 배포 패키지 생성

```batch
# 선택: 4
# 동작:
#   1. 컴파일 (옵션 2)
#   2. PingMonitor_v2.6_Release 폴더 생성
#   3. 필수 파일 복사
#   4. ZIP 압축 파일 생성
```

**생성 파일:**

```
PingMonitor_v2.6_Release/      # 폴더
PingMonitor_v2.6_Release.zip   # 압축 파일
```

---

## 🔨 수동 컴파일

### 전체 과정

```batch
cd main

REM 1. 컴파일
gcc -c ping_monitor_webview.c -o ping_monitor_webview.o -municode -mwindows
gcc -c module\config.c -o module_config.o
gcc -c module\network.c -o module_network.o
gcc -c module\notification.c -o module_notification.o
gcc -c module\port.c -o module_port.o
gcc -c outage.c -o outage.o

REM 2. 링킹
gcc -o ping_monitor.exe ping_monitor_webview.o module_config.o module_network.o module_notification.o module_port.o outage.o module\tray.c http_server.c browser_monitor.c config_api.c -municode -mwindows -lws2_32 -liphlpapi -lshlwapi -lshell32 -lole32 -loleaut32 -luuid -lgdi32

REM 3. 루트로 이동
move ping_monitor.exe ..

REM 4. 오브젝트 파일 삭제
del *.o
```

### 컴파일 플래그 설명

| 플래그 | 설명 |
|--------|------|
| `-municode` | UNICODE 진입점 사용 (wWinMain) |
| `-mwindows` | GUI 애플리케이션 (콘솔 창 숨김) |
| `-mconsole` | 콘솔 애플리케이션 (디버그용) |
| `-O2` | 최적화 레벨 2 |
| `-g` | 디버그 심볼 포함 |

### 링킹 라이브러리

| 라이브러리 | 용도 |
|------------|------|
| `-lws2_32` | Winsock 2 (네트워크) |
| `-liphlpapi` | IP Helper API (ICMP) |
| `-lshlwapi` | Shell Light-weight Utility (경로) |
| `-lshell32` | Shell API (브라우저 오픈) |
| `-lole32` | OLE (WebView2) |
| `-loleaut32` | OLE Automation |
| `-luuid` | UUID 생성 |
| `-lgdi32` | GDI (폰트, 그래픽) |

---

## 📥 Chart.js 다운로드

### 자동 다운로드 (권장)

```batch
DOWNLOAD_CHARTJS.bat
```

**동작:**

1. PowerShell로 Chart.js 다운로드
2. `web/chart.umd.min.js` 저장
3. 파일 크기 확인 (약 205KB)

### 수동 다운로드

```
URL: https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
저장 위치: web/chart.umd.min.js
```

**확인:**

```batch
dir web\chart.umd.min.js

# 출력 예시:
# 2025-02-01  14:30           205,240 chart.umd.min.js
```

---

## 📦 배포 패키지 생성

### 1. build.bat로 생성 (권장)

```batch
build.bat
# 선택: 4
```

### 2. 생성 내용

```
PingMonitor_v2.6_Release/
├── ping_monitor.exe
├── config/
│   ├── ping_config.ini
│   └── int_config.ini
├── data/                 # 빈 폴더
│   └── .gitkeep
└── web/
    ├── graph.html
    ├── chart.umd.min.js
    └── css/
        └── (8개 CSS 파일)
```

### 3. 압축 파일

```
PingMonitor_v2.6_Release.zip  # PowerShell Compress-Archive로 생성
```

---

## 💾 다른 PC에 설치

### 1. 압축 파일 전송

```batch
# USB, 이메일, 네트워크 드라이브 등으로 전송
PingMonitor_v2.6_Release.zip
```

### 2. 압축 해제

**중요: 영문 경로에 배치**

```batch
# ✅ 올바른 경로
C:\PingMonitor\
D:\Tools\PingMonitor\

# ❌ 잘못된 경로 (한글)
C:\Downloads\핑_모니터\
C:\프로그램\Ping Monitor\
```

### 3. 설정 파일 수정

```batch
# config/ping_config.ini 편집
notepad config\ping_config.ini
```

```ini
[Targets]
8.8.8.8,Google DNS
1.1.1.1,Cloudflare DNS
192.168.0.1,Gateway
```

### 4. 실행

```batch
# 더블클릭 또는 명령줄
ping_monitor.exe
```

**첫 실행 시 동작:**

1. HTTP 서버 시작 (포트 8080)
2. 시스템 트레이 아이콘 생성
3. 기본 브라우저 자동 오픈
4. 모니터링 시작

---

## 🔄 업그레이드

### v2.5에서 v2.6으로

#### 1. 기존 데이터 백업

```batch
copy config\ping_config.ini config\ping_config.ini.bak
copy data\notification_log.json data\notification_log.json.bak
```

#### 2. 새 파일 배치

```batch
# v2.6 압축 해제
# 기존 폴더에 덮어쓰기
```

#### 3. 설정 파일 확인

```batch
notepad config\ping_config.ini
notepad config\int_config.ini
```

**v2.6 신규 섹션 (선택사항):**

```ini
[OutageDetection]
OutageThreshold=300

[IPGroups]
8.8.8.8=DNS,1
```

#### 4. 프로그램 재시작

```batch
ping_monitor.exe
```

#### 5. 웹 대시보드 확인

- 장애 현황 탭 확인
- 설정 UI (⚙️ 버튼) 확인
- 최소화 버튼 (▲) 확인

---

## 🐛 빌드 문제 해결

### "gcc: command not found"

**원인:** MinGW 미설치 또는 PATH 미설정

**해결:**

```batch
# 1. gcc 위치 확인
where gcc

# 2. 출력 없으면 PATH 추가
set PATH=%PATH%;C:\mingw64\bin

# 3. 새 CMD 창 열기
```

### "undefined reference to `CreateFontW'"

**원인:** `-lgdi32` 라이브러리 누락

**해결:**

```batch
# build.bat 확인
# 링킹 명령에 -lgdi32 추가되어 있는지 확인
```

### "multiple definition of ..."

**원인:** 중복 파일 또는 백업 파일

**해결:**

```batch
cd main
del *_backup.c
del *.o
```

### "필수 파일 누락"

**원인:** web 폴더 또는 CSS 파일 없음

**해결:**

```batch
# 1. 필수 파일 확인
dir web\graph.html
dir web\css\*.css

# 2. 없으면 GitHub에서 다운로드
git clone https://github.com/zzangae/pings.git
```

---

## 빌드 성공 확인

### 1. 파일 생성 확인

```batch
dir ping_monitor.exe

# 출력 예시:
# 2025-02-01  14:30           150,513 ping_monitor.exe
```

### 2. 실행 테스트

```batch
ping_monitor.exe

# 예상 동작:
# - 트레이 아이콘 생성
# - 브라우저 자동 오픈
# - 콘솔 메시지 (디버그 모드)
```

### 3. 대시보드 접속

```
브라우저에서:
http://localhost:8080/web/graph.html

확인 사항:
- IP 카드 표시
- 실시간 차트 업데이트
- 통계 정보 표시
```

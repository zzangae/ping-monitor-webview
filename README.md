# 🌐 Ping Monitor WebView2 v2.7

Windows용 실시간 네트워크 핑 모니터링 도구 with 웹 대시보드

![Version](https://img.shields.io/badge/version-2.7-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0)
![Uptime](https://img.shields.io/badge/uptime-365%20days-green)

**GitHub**: https://github.com/zzangae/pings

## 목차

- [시스템 아키텍처](docs/ARCHITECTURE.md)
- [파일 구조 및 배포](docs/FILE_STRUCTURE.md)
- [설치 및 빌드 가이드](docs/INSTALLATION.md)
- [사용자 가이드](docs/USER_GUIDE.md)
- [설정 파일 상세](docs/CONFIGURATION.md)
- [문제 해결](docs/TROUBLESHOOTING.md)
- [버전 변경 이력](docs/CHANGELOG.md)

---

## 프로젝트 개요

**Ping Monitor WebView2**는 C 백엔드와 HTML/JavaScript 프론트엔드로 구성된 Windows용 네트워크 모니터링 도구입니다.

### ✨ v2.7 하이라이트: 365일 무중단 운영

- 🔄 **24시간 자동 핸들 갱신** - ICMP 핸들 장기 운영 안정성
- 🛡️ **SEH 크래시 핸들러** - 예외 발생 시 자동 재시작
- 📊 **프론트엔드 자동 리로드** - 메모리 누수 방지
- 📁 **로그 로테이션** - 5MB 제한, 자동 백업/정리

---

## 1. 시스템 아키텍처

### 1.1 전체 구조도

```
┌─────────────────────────────────────────────────────────┐
│                Ping Monitor v2.7 Architecture            │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌─────────────────┐        ┌──────────────────────┐   │
│  │   C Backend     │◄──────►│   Embedded HTTP      │   │
│  │                 │        │   Server             │   │
│  │ • ICMP Ping     │        │ • Port 8080-8099     │   │
│  │ • Network Loop  │        │ • Static Files       │   │
│  │ • Data Gen      │        │ • JSON API           │   │
│  │ • Outage Track  │        │ • /shutdown          │   │
│  │ • Custom Alert  │        └──────────────────────┘   │
│  │ • 365일 워치독  │                   │                │
│  └─────────────────┘                   │                │
│           │                            │                │
│           ▼                            ▼                │
│  ┌─────────────────┐        ┌──────────────────────┐   │
│  │   Data Layer    │        │   Web Frontend       │   │
│  │                 │        │                      │   │
│  │ • JSON Files    │◄──────►│ • HTML/CSS/JS        │   │
│  │ • INI Configs   │        │ • Chart.js           │   │
│  │ • Atomic Write  │        │ • Real-time Update   │   │
│  │ • Log Rotation  │        │ • Auto Reload        │   │
│  └─────────────────┘        └──────────────────────┘   │
│                                                           │
│  ┌─────────────────────────────────────────────────┐   │
│  │            System Tray Integration               │   │
│  │  • Start/Stop  • Browser  • Notifications        │   │
│  │  • Reload Config  • Change Port  • Exit          │   │
│  └─────────────────────────────────────────────────┘   │
│                                                           │
│  ┌─────────────────────────────────────────────────┐   │
│  │         v2.7 365일 무중단 운영 시스템            │   │
│  │  • SEH 크래시 핸들러  • 자동 재시작              │   │
│  │  • ICMP 핸들 갱신     • 로그 로테이션            │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 1.2 핵심 컴포넌트

#### 1.2.1 C Backend

- **언어**: C (GCC 14.2.0)
- **플랫폼**: Windows (MinGW-w64)
- **주요 기능**:
  - ICMP 에코 요청/응답 처리
  - 멀티 IP 동시 모니터링
  - 장애 감지 및 복구 추적
  - 커스텀 알림 창 시스템
  - 설정 파일 파싱
  - **v2.7**: 정적 리소스 재사용 (핸들, 버퍼)
  - **v2.7**: 24시간 워치독 (핸들 자동 갱신)
  - **v2.7**: SEH 크래시 핸들러

#### 1.2.2 Embedded HTTP Server

- **포트 범위**: 8080-8099 (자동 탐색)
- **Document Root**: 실행 파일 위치
- **엔드포인트**:
  - `GET /web/graph.html` - 메인 대시보드
  - `GET /data/ping_data.json` - 실시간 핑 데이터
  - `GET /data/notification_log.json` - 알림 로그
  - `GET /data/outage_log.json` - 장애 로그
  - `GET /config/ping_config.ini` - 설정 파일 (읽기 전용)
  - `POST /shutdown` - 서버 종료

#### 1.2.3 Web Frontend

- **기술 스택**:
  - HTML5 + CSS3
  - Vanilla JavaScript (ES6+)
  - Chart.js 4.4.0 (로컬 호스팅)
- **주요 기능**:
  - 실시간 차트 시각화
  - IP 비교 타임라인
  - 알림/장애 현황 필터링
  - 그룹 관리 및 임계값 설정
  - LocalStorage 상태 저장
  - **v2.7**: 24시간 자동 리로드
  - **v2.7**: 메모리 사용량 모니터링

#### 1.2.4 Custom Notification System

- **특징**: Outlook 스타일 커스텀 알림 창
- **위치**: 화면 우하단 (트레이 아이콘 가리지 않음)
- **애니메이션**: 페이드 인/아웃
- **자동 닫힘**: 5초 후
- **스레드 안전**: PostMessage를 통한 메인 스레드 처리

#### 1.2.5 v2.7 365일 무중단 운영 시스템

| 구성 요소         | 설명                     | 주기       |
| ----------------- | ------------------------ | ---------- |
| ICMP 워치독       | 핸들 자동 갱신           | 24시간     |
| 실패 감지         | 연속 실패 시 핸들 재생성 | 100회 실패 |
| SEH 핸들러        | 크래시 시 자동 재시작    | 즉시       |
| 프론트엔드 리로드 | 메모리 정리              | 24시간     |
| 메모리 체크       | 임계값 초과 시 리로드    | 30분       |
| 로그 로테이션     | 파일 크기 관리           | 1시간      |

### 1.3 데이터 흐름

```
[Config Files] → [C Backend] → [JSON Files] → [Web Frontend]
      ↓              ↓              ↑              ↓
 ping_config.ini   ICMP Ping   HTTP Server    Chart.js
 int_config.ini    Network      (localhost)   Dashboard
                   Monitoring
                      ↓
              [Log Rotation]
              • 5MB 제한
              • 3개 백업
              • 30일 보관
```

### 1.4 모듈 구조

```
ping_monitor_webview.c (메인)
├── module/
│   ├── types.h          (타입 정의)
│   ├── config.c/h       (설정 파일 로딩)
│   ├── network.c/h      (네트워크 모니터링 + v2.7 워치독)
│   ├── notification.c/h (커스텀 알림)
│   ├── port.c/h         (포트 관리 다이얼로그)
│   └── tray.c/h         (시스템 트레이)
├── http_server.c/h      (HTTP 서버)
├── outage.c/h           (장애 추적 + v2.7 로그 로테이션)
├── browser_monitor.c/h  (브라우저 감지)
└── config_api.c/h       (설정 API)
```

---

## 2. 파일 구조 및 배포

### 2.1 개발 환경 구조

```
ping_monitor_v27/
├── ping_monitor.exe          # 실행 파일 (빌드 후 생성)
├── build.bat                 # 빌드 스크립트
├── DOWNLOAD_CHARTJS.bat      # Chart.js 다운로드
├── README.md
├── DOCUMENTATION_v2.7.md
├── GRAPH_HTML_SUMMARY.md
│
├── main/                     # C 소스 코드
│   ├── ping_monitor_webview.c  (v2.7: SEH 크래시 핸들러)
│   ├── http_server.c/h
│   ├── outage.c/h              (v2.7: 로그 로테이션)
│   ├── browser_monitor.c/h
│   ├── config_api.c/h
│   └── module/
│       ├── types.h
│       ├── config.c/h
│       ├── network.c/h         (v2.7: 워치독, 정적 리소스)
│       ├── notification.c/h
│       ├── port.c/h
│       └── tray.c/h
│
├── config/                   # 설정 파일
│   ├── ping_config.ini      # 공개 IP 설정
│   └── int_config.ini       # 내부 IP 설정
│
├── data/                     # 자동 생성 데이터 (실행 시)
│   ├── ping_data.json
│   ├── notification_log.json
│   ├── outage_log.json
│   └── crash_log.txt         # v2.7: 크래시 로그
│
└── web/                      # 웹 파일
    ├── graph.html            # v2.7: 자동 리로드 기능
    ├── chart.umd.min.js     # 205KB
    └── css/
        ├── variables.css
        ├── base.css
        ├── components.css
        ├── dashboard.css
        ├── notifications.css
        ├── outages.css
        ├── settings.css
        └── responsive.css
```

### 2.2 배포 패키지 구조

```
PingMonitor_v2.7_Release/
├── ping_monitor.exe
├── config/
│   ├── ping_config.ini
│   └── int_config.ini
├── data/                     # 빈 폴더 (실행 시 생성)
│   └── .gitkeep
└── web/
    ├── graph.html
    ├── chart.umd.min.js
    └── css/
        └── (8개 CSS 파일)
```

### 2.3 실행 파일 크기

- **ping_monitor.exe**: ~160KB (v2.7 기능 추가)
- **chart.umd.min.js**: ~205KB
- **총 배포 패키지**: ~550KB (압축 시 ~350KB)

---

## 3. 설치 및 빌드 가이드

### 3.1 필수 요구사항

#### 3.1.1 개발 환경

- **OS**: Windows 10/11 (64-bit)
- **컴파일러**: MinGW-w64 GCC 14.2.0 이상
- **빌드 도구**: GNU Make (선택사항)

#### 3.1.2 런타임 요구사항

- **OS**: Windows 10/11
- **브라우저**: Chrome, Edge, Firefox (최신 버전)
- **권한**: 관리자 권한 (ICMP 핑 사용 시 필요할 수 있음)

### 3.2 MinGW 설치

#### 3.2.1 다운로드

```
https://github.com/niXman/mingw-builds-binaries/releases
→ x86_64-14.2.0-release-posix-seh-ucrt-rt_v12-rev0.7z
```

#### 3.2.2 설치

1. 압축 해제: `C:\mingw64`
2. 환경 변수 추가: `PATH`에 `C:\mingw64\bin` 추가
3. 확인:

```cmd
gcc --version
```

### 3.3 빌드 방법

#### 3.3.1 build.bat 사용 (권장)

```cmd
cd ping_monitor_v27
build.bat
```

**메뉴 선택:**

```
1. 컴파일 및 실행
2. 컴파일만
3. 디버그 모드 (콘솔 출력 + 로그 파일)
4. 배포 패키지 생성
5. 종료
```

#### 3.3.2 수동 컴파일

```cmd
cd main

REM 컴파일
gcc -c ping_monitor_webview.c -o ping_monitor_webview.o -municode -mwindows
gcc -c module\config.c -o module_config.o
gcc -c module\network.c -o module_network.o
gcc -c module\notification.c -o module_notification.o
gcc -c module\port.c -o module_port.o
gcc -c outage.c -o outage.o

REM 링킹
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

REM 루트로 이동
move ping_monitor.exe ..
```

#### 3.3.3 Chart.js 다운로드

```cmd
DOWNLOAD_CHARTJS.bat
```

→ `web/chart.umd.min.js` 생성

### 3.4 배포 패키지 생성

```cmd
build.bat
→ 4 선택
```

**생성 파일:**

- `PingMonitor_v2.7_Release/` (폴더)
- `PingMonitor_v2.7_Release.zip` (압축)

---

## 4. 사용자 가이드

### 4.1 첫 실행

#### 4.1.1 설정 파일 준비

1. `config/ping_config.ini` 편집
2. 모니터링할 IP 추가:

```ini
[Targets]
8.8.8.8,Google DNS
1.1.1.1,Cloudflare DNS
```

#### 4.1.2 실행

```cmd
ping_monitor.exe
```

또는 더블클릭

#### 4.1.3 자동 동작

1. HTTP 서버 시작 (포트 8080-8099 자동 탐색)
2. 시스템 트레이 아이콘 생성
3. 기본 브라우저로 대시보드 오픈
4. 백그라운드에서 모니터링 시작
5. **v2.7**: 365일 무중단 운영 모드 활성화

### 4.2 시스템 트레이 메뉴

**트레이 아이콘 우클릭:**

```
🟢 Ping Monitor v2.7
├─ ▶ 시작 / ⏸ 일시정지
├─ 🌐 웹 대시보드 열기
├─ 🔔 알림 활성화 ☑
├─ 🔄 설정 다시 불러오기
├─ 🔧 서버 포트 변경
└─ ❌ 종료
```

**트레이 아이콘 더블클릭:**

- 웹 대시보드 오픈

### 4.3 웹 대시보드 사용법

#### 4.3.1 탭 구조

- **📊 대시보드**: 실시간 모니터링
- **🔔 알림 기록**: 타임아웃/복구 알림
- **🔥 장애 현황**: 장애 발생/복구 이력

#### 4.3.2 대시보드 기능

**통합 컨트롤 바:**

- 현재 시간/날짜
- 모니터링 시간 (uptime)
- 일시정지/시작 버튼
- 새로고침 버튼
- 통계 (전체/온라인/오프라인/평균 지연시간)

**IP 비교 타임라인:**

- 여러 IP의 지연시간을 실시간 그래프로 비교
- ⚙️ 설정 버튼:
  - 📊 차트 설정: 표시할 IP 선택
  - ⏱️ 그룹별 임계값: 경고/위험 레벨 설정
  - 📋 IP 그룹 관리: IP 그룹화
- ▲ 최소화 버튼: 차트만 숨기기/표시

**IP 카드:**

- 각 IP별 실시간 정보
  - 온라인/오프라인 상태 (🟢/🔴)
  - 현재 지연시간
  - 성공률
  - 연속 실패 횟수
  - 미니 차트
- ⚙️ 설정: 우선순위/그룹 변경
- ▼ 최소화: 작은 아이콘으로 축소

**최소화된 IP 영역:**

- 최소화된 IP 아이콘 표시
- 클릭 시 원래 크기로 복원
- ▲ 최소화 버튼: 아이콘들 숨기기/표시

#### 4.3.3 알림 기록

**필터:**

- 타입: 전체/타임아웃/복구
- 날짜: 오늘/어제/최근 7일/최근 30일/전체

**테이블:**

```
시간      | 타입    | IP 이름      | IP 주소      | 날짜
14:30:25 | timeout | Google DNS  | 8.8.8.8     | 2024-02-01
14:35:10 | recovery| Google DNS  | 8.8.8.8     | 2024-02-01
```

#### 4.3.4 장애 현황

**필터:**

- 상태: 전체/진행중/복구완료
- 날짜: 오늘/어제/최근 7일/최근 30일/전체

**테이블:**

```
시작 시간         | IP 이름     | IP 주소   | 상태    | 지속 시간 | 종료 시간
2024-02-01 14:30 | Google DNS | 8.8.8.8  | 복구완료 | 4분 45초  | 2024-02-01 14:35
```

### 4.4 커스텀 알림 창

**특징:**

- Outlook 스타일 디자인
- 화면 우하단 표시 (트레이 아이콘 가리지 않음)
- 5초 후 자동 닫힘
- 클릭 시 즉시 닫힘
- 최대 5개 동시 표시 (자동 정렬)

**알림 조건:**

- 타임아웃: 연속 3회 실패 시
- 복구: 오프라인 → 온라인 전환 시

**알림 제어:**

- 트레이 메뉴에서 활성화/비활성화
- 비활성화 시 현재 표시된 알림 모두 닫힘

### 4.5 설정 관리

#### 4.5.1 IP 추가/수정

1. `config/ping_config.ini` 편집
2. 트레이 메뉴 → "설정 다시 불러오기"
3. 자동으로 새 IP 모니터링 시작

#### 4.5.2 포트 변경

1. 트레이 메뉴 → "서버 포트 변경"
2. 새 포트 입력 (8080-8099)
3. 자동 재시작

#### 4.5.3 데이터 초기화

- `data/` 폴더 삭제 후 재실행
- 알림/장애 로그만 초기화: 해당 JSON 파일 삭제

### 4.6 v2.7 365일 무중단 운영

#### 4.6.1 자동 운영 기능

| 기능              | 설명                      | 사용자 개입 |
| ----------------- | ------------------------- | ----------- |
| ICMP 핸들 갱신    | 24시간마다 자동           | 불필요      |
| 연속 실패 복구    | 100회 실패 시 핸들 재생성 | 불필요      |
| 크래시 자동 복구  | SEH 예외 감지 시 재시작   | 불필요      |
| 프론트엔드 리로드 | 24시간마다 자동           | 불필요      |
| 메모리 관리       | 500MB 초과 시 리로드      | 불필요      |
| 로그 로테이션     | 5MB 초과 시 자동 백업     | 불필요      |

#### 4.6.2 크래시 로그 확인

크래시 발생 시 `data/crash_log.txt`에 기록:

```
[2025-02-12 14:30:25] 크래시 발생 - 예외 코드: 0xC0000005, 주소: 0x00401234
```

#### 4.6.3 로그 파일 관리

**자동 로테이션:**

- 로그 파일이 5MB 초과 시 자동 백업
- `outage_log.json` → `outage_log.json.1` → `.2` → `.3`
- 최대 3개 백업 유지
- 30일 이상 된 백업 자동 삭제

---

## 5. 설정 파일 상세

### 5.1 ping_config.ini (공개 IP)

#### 5.1.1 기본 구조

```ini
[Settings]
NotificationsEnabled=1
NotificationCooldown=300
NotifyOnTimeout=1
NotifyOnRecovery=1
ConsecutiveFailures=3
OutageThreshold=5

[Targets]
8.8.8.8,Google DNS,1,DNS
1.1.1.1,Cloudflare DNS,1,DNS
208.67.222.222,OpenDNS,2,DNS
```

#### 5.1.2 Settings 섹션

| 키                     | 설명                | 기본값 | 범위             |
| ---------------------- | ------------------- | ------ | ---------------- |
| `NotificationsEnabled` | 알림 활성화         | 1      | 0=비활성, 1=활성 |
| `NotificationCooldown` | 알림 쿨다운 (초)    | 300    | 60-3600          |
| `NotifyOnTimeout`      | 타임아웃 알림       | 1      | 0/1              |
| `NotifyOnRecovery`     | 복구 알림           | 1      | 0/1              |
| `ConsecutiveFailures`  | 알림 실패 임계값    | 3      | 1-10             |
| `OutageThreshold`      | 장애 인정 시간 (분) | 5      | 1-60             |

#### 5.1.3 Targets 섹션

**형식:**

```
IP주소,이름,우선순위,그룹
```

**예제:**

```ini
8.8.8.8,Google DNS,1,DNS
192.168.1.1,Gateway,1,Network
10.0.0.100,Web Server,2,Servers
```

**필드 설명:**

- **IP주소**: IPv4 주소
- **이름**: 표시 이름 (공백 가능)
- **우선순위**: 1=높음, 2=보통, 3=낮음
- **그룹**: 그룹 이름 (선택사항)

### 5.2 int_config.ini (내부 IP)

#### 5.2.1 용도

- 내부 네트워크 IP 분리 관리
- 보안상 공개 설정과 분리

#### 5.2.2 형식

```ini
[Targets]
192.168.1.1,Router,1,Network
192.168.1.10,NAS,2,Storage
10.0.0.50,Database Server,1,Database
```

**주의:**

- Settings 섹션 없음 (ping_config.ini 공유)
- Targets 섹션만 사용

### 5.3 설정 적용 방법

#### 5.3.1 실시간 적용

```
1. 설정 파일 수정
2. 저장
3. 트레이 메뉴 → "설정 다시 불러오기"
```

#### 5.3.2 재시작 필요 없음

- 모든 설정은 런타임 리로드 가능
- IP 추가/삭제 즉시 반영

---

## 6. 문제 해결

### 6.1 컴파일 오류

#### 6.1.1 "gcc: command not found"

**원인**: MinGW 미설치 또는 PATH 미설정
**해결:**

```cmd
where gcc
```

출력 없으면:

1. MinGW 설치 확인
2. 환경 변수 PATH에 `C:\mingw64\bin` 추가
3. 새 CMD 창 열기

#### 6.1.2 "undefined reference to ..."

**원인**: 링킹 라이브러리 누락
**해결:**

- `-lgdi32` 추가 (CreateFontW 오류)
- `-lws2_32` 추가 (Winsock 오류)
- `-lshlwapi` 추가 (PathRemoveFileSpecW 오류)

#### 6.1.3 "multiple definition of ..."

**원인**: 백업 파일이나 중복 파일 존재
**해결:**

```cmd
del main\*_backup.c
del main\*.o
```

### 6.2 실행 오류

#### 6.2.4 "필수 파일 누락"

**원인**: web 폴더 또는 CSS 파일 없음
**해결:**

1. `web/graph.html` 존재 확인
2. `web/css/*.css` (8개 파일) 확인
3. `web/chart.umd.min.js` 확인
   - 없으면 `DOWNLOAD_CHARTJS.bat` 실행

#### 6.2.5 "HTTP 서버 시작 실패"

**원인**: 포트 8080-8099 모두 사용 중
**해결:**

```cmd
netstat -ano | findstr :8080
taskkill /PID <PID> /F
```

#### 6.2.6 "설정 파일에 IP가 없습니다"

**원인**: ping_config.ini 또는 int_config.ini에 [Targets] 섹션 없음
**해결:**

```ini
[Targets]
8.8.8.8,Google DNS
```

최소 1개 IP 추가

### 6.3 웹 대시보드 오류

#### 6.3.1 차트가 로딩되지 않음

**원인**: Chart.js 파일 없음
**해결:**

```cmd
DOWNLOAD_CHARTJS.bat
```

→ `web/chart.umd.min.js` 생성 확인 (205KB)

#### 6.3.2 데이터가 표시되지 않음

**원인**: data 폴더 없음 또는 권한 문제
**해결:**

```cmd
mkdir data
```

관리자 권한으로 실행

#### 6.3.3 CSS가 깨짐

**원인**: CSS 파일 경로 오류
**해결:**

- 브라우저 개발자 도구 (F12) → Network 탭 확인
- 404 오류 파일 확인
- `web/css/` 폴더에 8개 CSS 파일 모두 있는지 확인

### 6.4 알림 오류

#### 6.4.1 알림창이 나타나지 않음

**원인**: 알림 비활성화 또는 임계값 미도달
**해결:**

1. 트레이 메뉴 → "알림 활성화" 체크 확인
2. `ping_config.ini`:

```ini
ConsecutiveFailures=1  (테스트용)
```

#### 6.4.2 알림창이 무한 로딩

**원인**: 폰트 생성 오류 (v2.6에서 수정됨)
**해결:**

- 최신 버전 사용
- 또는 `-lgdi32` 링킹 확인

#### 6.4.3 알림창이 트레이 아이콘 가림

**원인**: 구버전 사용 (v2.6 이전)
**해결:**

- v2.6 이상 업그레이드 (커스텀 알림 창)

### 6.5 성능 문제

#### 6.5.1 CPU 사용률 높음

**원인**: IP 개수 과다 또는 업데이트 간격 짧음
**해결:**

- IP 개수 50개 이하 권장
- 업데이트 간격 1초 권장 (현재 기본값)

#### 6.5.2 메모리 누수

**원인**: 차트 인스턴스 미정리
**해결:**

- **v2.7**: 24시간마다 자동 리로드로 해결
- 또는 브라우저 새로고침 (F5)

### 6.6 v2.7 365일 운영 관련

#### 6.6.1 자동 재시작 반복

**원인**: 반복적인 크래시 발생
**해결:**

1. `data/crash_log.txt` 확인
2. 예외 코드로 원인 파악
3. 최대 5회 재시작 후 수동 개입 필요

#### 6.6.2 로그 파일 과다

**원인**: 로테이션 실패 또는 비정상 종료
**해결:**

```cmd
del data\*.json.1
del data\*.json.2
del data\*.json.3
```

---

## 7. 버전 변경 이력

### v2.7 (2025-02-12) - 365일 무중단 운영

#### 7.1 주요 신규 기능

**365일 무중단 운영 시스템**

- SEH 크래시 핸들러: 예외 발생 시 자동 재시작
- 크래시 로그 기록: `data/crash_log.txt`
- 최대 5회 자동 재시작, 쿨다운 60초
- 재시작 플래그 파일로 상태 추적

**C 백엔드 워치독**

- ICMP 핸들 24시간 주기 자동 갱신
- 연속 100회 실패 시 핸들 재생성
- 정적 리소스 재사용 (핸들, 버퍼)
- malloc/free 반복 제거로 메모리 단편화 방지

**프론트엔드 자동 리로드**

- 24시간마다 페이지 자동 리로드
- 메모리 500MB 초과 시 강제 리로드
- 30분마다 메모리 사용량 체크
- localStorage로 상태 유지

**로그 로테이션**

- 최대 파일 크기: 5MB
- 백업 파일: 최대 3개 (.1, .2, .3)
- 30일 이상 백업 자동 삭제
- 1시간마다 로테이션 체크

#### 7.2 아키텍처 개선

**network.c 최적화**

```c
// 정적 리소스 (프로그램 수명 동안 유지)
static HANDLE s_hIcmpFile = INVALID_HANDLE_VALUE;
static LPVOID s_replyBuffer = NULL;
static wchar_t *s_jsonBuffer = NULL;

// 워치독 상태
static time_t s_lastHandleRefresh = 0;
static int s_consecutivePingFailures = 0;
```

**outage.c 로테이션**

```c
#define MAX_LOG_SIZE_MB     5
#define MAX_BACKUP_COUNT    3
#define BACKUP_RETENTION_DAYS 30
#define ROTATION_CHECK_INTERVAL 3600
```

**ping_monitor_webview.c 크래시 핸들러**

```c
LONG WINAPI CrashHandler(EXCEPTION_POINTERS *exInfo)
{
    // 크래시 로그 기록
    // 정리 작업
    // 자동 재시작
}
```

#### 7.3 신규 함수

| 파일       | 함수                        | 설명               |
| ---------- | --------------------------- | ------------------ |
| network.c  | `InitNetworkModule()`       | 정적 리소스 초기화 |
| network.c  | `CleanupNetworkModule()`    | 리소스 정리        |
| network.c  | `RefreshIcmpHandle()`       | ICMP 핸들 갱신     |
| network.c  | `CheckAndRefreshHandle()`   | 워치독 체크        |
| outage.c   | `CheckAllLogsRotation()`    | 로그 로테이션      |
| outage.c   | `ForceLogRotation()`        | 강제 로테이션      |
| outage.c   | `CleanupOldBackups()`       | 오래된 백업 삭제   |
| main       | `CrashHandler()`            | SEH 예외 처리      |
| main       | `CreateRestartFlag()`       | 재시작 플래그 생성 |
| main       | `RequestRestart()`          | 프로세스 재시작    |
| graph.html | `initContinuousOperation()` | 무중단 운영 초기화 |
| graph.html | `performSafeReload()`       | 안전한 리로드      |
| graph.html | `checkMemoryUsage()`        | 메모리 체크        |

#### 7.4 알려진 제한사항

- Windows 전용 (Linux/macOS 미지원)
- IPv4만 지원 (IPv6 미지원)
- 최대 IP 개수: 약 100개 (성능 고려)
- 브라우저 자동 닫기 기능 비활성화 (사용자 편의)
- 자동 재시작 최대 5회 (무한 루프 방지)

### v2.6 (2025-02-01)

#### 주요 기능

- 커스텀 알림 시스템 (Outlook 스타일)
- 최소화 기능 개선
- 배포 시스템 (build.bat)

#### 버그 수정

- `-lgdi32` 라이브러리 누락 수정
- 알림창 무한 로딩 수정
- 스레드 안전성 개선

---

## 부록

### A. 라이브러리 의존성

```
• ws2_32.lib     - Winsock 2 (네트워크)
• iphlpapi.lib   - IP Helper API (ICMP)
• shlwapi.lib    - Shell Light-weight Utility (경로)
• shell32.lib    - Shell API (브라우저 오픈)
• ole32.lib      - OLE (WebView2)
• oleaut32.lib   - OLE Automation
• uuid.lib       - UUID 생성
• gdi32.lib      - GDI (폰트, 그래픽)
```

### B. 포트 번호 범위

```
8080 - 기본 포트
8081 - 8099 - 자동 탐색 범위
```

### C. 파일 확장자

```
.c    - C 소스 코드
.h    - 헤더 파일
.o    - 오브젝트 파일 (컴파일 중간 산출물)
.exe  - 실행 파일
.ini  - 설정 파일
.json - 데이터 파일
.html - 웹 페이지
.css  - 스타일시트
.js   - JavaScript
.md   - 마크다운 문서
.bat  - 배치 스크립트
.txt  - 텍스트 파일 (v2.7: 크래시 로그)
.flag - 플래그 파일 (v2.7: 재시작 플래그)
```

### D. 기본 키보드 단축키

```
F5          - 대시보드 새로고침
F11         - 전체 화면
F12         - 개발자 도구
Ctrl+F5     - 캐시 무시 새로고침
Alt+F4      - 브라우저 닫기 (프로그램은 계속 실행)
```

### E. v2.7 365일 운영 체크리스트

```
□ 정적 리소스 초기화 확인
□ 워치독 로그 확인 (콘솔)
□ 크래시 로그 파일 모니터링
□ 로그 파일 크기 확인
□ 브라우저 메모리 사용량 확인
□ 자동 리로드 동작 확인
```

### F. 참고 자료

- GitHub Repository: https://github.com/zzangae/pings
- Chart.js Documentation: https://www.chartjs.org/docs/latest/
- MinGW-w64: https://www.mingw-w64.org/
- Winsock API: https://docs.microsoft.com/en-us/windows/win32/winsock/
- SEH (Structured Exception Handling): https://docs.microsoft.com/en-us/cpp/cpp/structured-exception-handling-c-cpp

---

**문서 버전**: v2.7
**최종 수정일**: 2025-02-12

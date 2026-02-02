# 3. FILE_STRUCTURE.md

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

- 프로젝트 디렉토리 구조
- 각 파일/폴더 설명
- 배포 패키지 구조
- 런타임 생성 파일
- 파일 크기 정보

---

## 📁 개발 환경 구조

```
ping_monitor_v26/
├── ping_monitor.exe          # 실행 파일 (빌드 후 생성, ~150KB)
├── build.bat                 # 빌드 스크립트
├── DOWNLOAD_CHARTJS.bat      # Chart.js 다운로드
├── README.md
├── DOCUMENTATION_v2.6.md     # 완전 문서
├── GRAPH_HTML_SUMMARY.md     # graph.html 구조 요약
│
├── main/                     # C 소스 코드
│   ├── ping_monitor_webview.c     # 메인 프로그램
│   ├── http_server.c/h            # HTTP 서버 구현
│   ├── outage.c/h                 # 장애 관리
│   ├── browser_monitor.c/h        # 브라우저 모니터
│   ├── config_api.c/h             # 설정 API
│   └── module/
│       ├── types.h                # 전역 타입 정의
│       ├── config.c/h             # 설정 파일 로딩
│       ├── network.c/h            # ICMP 핑 모니터링
│       ├── notification.c/h       # 커스텀 알림 시스템
│       ├── port.c/h               # 포트 관리
│       └── tray.c/h               # 시스템 트레이
│
├── config/                   # 설정 파일
│   ├── ping_config.ini      # 공개 IP 설정
│   └── int_config.ini       # 내부 IP 설정
│
├── data/                     # 자동 생성 데이터 (실행 시)
│   ├── ping_data.json       # 실시간 핑 데이터
│   ├── notification_log.json# 알림 로그
│   └── outage_log.json      # 장애 로그
│
└── web/                      # 웹 파일
    ├── graph.html           # 대시보드 HTML (~80KB)
    ├── chart.umd.min.js     # Chart.js 라이브러리 (205KB)
    └── css/                 # CSS 파일 (8개)
        ├── variables.css    # CSS 변수 정의
        ├── base.css         # 기본 스타일
        ├── components.css   # 컴포넌트 스타일
        ├── dashboard.css    # 대시보드 레이아웃
        ├── notifications.css# 알림 탭 스타일
        ├── outages.css      # 장애 타� 스타일
        ├── settings.css     # 설정 UI 스타일
        └── responsive.css   # 반응형 디자인
```

---

## 📦 배포 패키지 구조

```
PingMonitor_v2.6_Release/
├── ping_monitor.exe          # 실행 파일
├── config/
│   ├── ping_config.ini      # 공개 IP 설정
│   └── int_config.ini       # 내부 IP 설정
├── data/                     # 빈 폴더 (실행 시 생성)
│   └── .gitkeep
└── web/
    ├── graph.html
    ├── chart.umd.min.js
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

**배포 패키지 크기:**
- 압축 전: ~500KB
- 압축 후 (ZIP): ~300KB

---

## 📄 파일별 상세 설명

### 실행 파일

| 파일                 | 크기    | 설명                      |
| -------------------- | ------- | ------------------------- |
| ping_monitor.exe     | ~150KB  | 메인 실행 파일            |

### 소스 코드 (main/)

| 파일                       | 라인 수 | 설명                          |
| -------------------------- | ------- | ----------------------------- |
| ping_monitor_webview.c     | ~420    | 메인 프로그램, 진입점         |
| http_server.c/h            | ~500    | 내장 HTTP 서버                |
| outage.c/h                 | ~200    | 장애 감지 및 로그 관리        |
| browser_monitor.c/h        | ~150    | 브라우저 프로세스 모니터링    |
| config_api.c/h             | ~100    | 설정 파일 API                 |
| module/types.h             | ~150    | 전역 타입 및 상수 정의        |
| module/config.c/h          | ~300    | INI 파일 파싱                 |
| module/network.c/h         | ~400    | ICMP 핑, 통계, JSON 생성      |
| module/notification.c/h    | ~350    | 커스텀 알림 창 시스템         |
| module/port.c/h            | ~150    | 포트 관리 다이얼로그          |
| module/tray.c/h            | ~200    | 시스템 트레이 아이콘          |

### 웹 파일 (web/)

| 파일                 | 크기    | 설명                          |
| -------------------- | ------- | ----------------------------- |
| graph.html           | ~80KB   | 대시보드 HTML/JavaScript      |
| chart.umd.min.js     | 205KB   | Chart.js 4.4.0 라이브러리     |
| css/variables.css    | 430B    | CSS 변수 (색상, 간격)         |
| css/base.css         | 770B    | 기본 스타일 (body, 폰트)      |
| css/components.css   | 4.2KB   | 버튼, 카드, 모달              |
| css/dashboard.css    | 11.7KB  | 대시보드 레이아웃             |
| css/notifications.css| 1.4KB   | 알림 탭 스타일                |
| css/outages.css      | 3.3KB   | 장애 탭 스타일                |
| css/settings.css     | 5.7KB   | 설정 UI 스타일                |
| css/responsive.css   | 4.6KB   | 반응형 미디어 쿼리            |

### 설정 파일 (config/)

| 파일             | 설명                  | Git 추적 |
| ---------------- | --------------------- | -------- |
| ping_config.ini  | 공개 IP 설정          | ✅       |
| int_config.ini   | 내부 IP 설정 (보안)   | ❌       |

### 데이터 파일 (data/) - 자동 생성

| 파일                  | 크기     | 갱신 주기 | 설명                  |
| --------------------- | -------- | --------- | --------------------- |
| ping_data.json        | 가변     | 1초       | 실시간 핑 데이터      |
| notification_log.json | 누적     | 이벤트 시 | 타임아웃/복구 알림    |
| outage_log.json       | 누적     | 이벤트 시 | 장애 발생/복구 이력   |

---

## 📊 데이터 형식

### ping_data.json

```json
[
  {
    "ip": "8.8.8.8",
    "name": "Google DNS",
    "online": 1,
    "latency": 25,
    "sent": 100,
    "received": 98,
    "lost": 2,
    "success_rate": 98.0,
    "consecutive_failures": 0,
    "avg_latency": 26,
    "min_latency": 20,
    "max_latency": 35,
    "priority": 1,
    "group": "DNS"
  }
]
```

### notification_log.json

```json
[
  {
    "type": "timeout",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "time": "14:30:25",
    "date": "2024-02-01"
  },
  {
    "type": "recovery",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "time": "14:35:10",
    "date": "2024-02-01"
  }
]
```

### outage_log.json

```json
[
  {
    "ip": "8.8.8.8",
    "name": "Google DNS",
    "start_time": "2024-02-01 14:30:25",
    "end_time": "2024-02-01 14:35:10",
    "duration_seconds": 285,
    "status": "복구완료"
  }
]
```

---

## 🔨 빌드 산출물

### 컴파일 시 생성되는 파일

```
main/
├── *.o                # 오브젝트 파일 (컴파일 중간 산출물)
└── ping_monitor.exe   # 실행 파일 (빌드 후 루트로 이동)
```

**주의:** `.o` 파일은 빌드 스크립트에서 자동 삭제됩니다.

---

## 🗂️ .gitignore 규칙

```gitignore
# 실행 파일
*.exe
ping_monitor

# 컴파일 산출물
*.o
*.obj

# 데이터 파일
data/
ping_data.json
notification_log.json
outage_log.json

# 내부 설정 (보안)
int_config.ini

# 로그
*.log
ping_monitor_debug.log

# 백업
*_backup.*
*.bak

# IDE
.vscode/
.idea/
*.swp
*~
```

---

## 📥 배포 파일 생성 방법

### build.bat 사용

```cmd
build.bat
→ 4 선택 (배포 패키지 생성)
```

**생성되는 파일:**
1. `PingMonitor_v2.6_Release/` 폴더
2. `PingMonitor_v2.6_Release.zip` 압축 파일

### 수동 배포 패키지 생성

```cmd
mkdir PingMonitor_v2.6_Release
mkdir PingMonitor_v2.6_Release\config
mkdir PingMonitor_v2.6_Release\data
mkdir PingMonitor_v2.6_Release\web
mkdir PingMonitor_v2.6_Release\web\css

copy ping_monitor.exe PingMonitor_v2.6_Release\
copy config\*.ini PingMonitor_v2.6_Release\config\
copy web\graph.html PingMonitor_v2.6_Release\web\
copy web\chart.umd.min.js PingMonitor_v2.6_Release\web\
xcopy /E /I web\css PingMonitor_v2.6_Release\web\css\

powershell Compress-Archive -Path PingMonitor_v2.6_Release -DestinationPath PingMonitor_v2.6_Release.zip
```

---

## 📋 체크리스트

### 배포 전 확인사항

- [ ] `ping_monitor.exe` 존재 (~150KB)
- [ ] `web/graph.html` 존재 (~80KB)
- [ ] `web/chart.umd.min.js` 존재 (205KB)
- [ ] `web/css/*.css` 8개 파일 모두 존재
- [ ] `config/ping_config.ini` 샘플 IP 포함
- [ ] `config/int_config.ini` 없음 (보안상 제외)
- [ ] `data/` 폴더 생성됨 (빈 폴더)
- [ ] `.gitkeep` 파일로 data 폴더 유지

### 실행 전 확인사항

- [ ] Windows 10/11 64-bit
- [ ] 브라우저 설치 (Chrome/Edge/Firefox)
- [ ] 포트 8080-8099 사용 가능
- [ ] 영문 경로에 배치
- [ ] 관리자 권한 (선택, ICMP 핑 시 필요할 수 있음)

---

## 🚀 다른 PC에 배포

### 1단계: 압축 파일 생성

```cmd
build.bat → 4번 선택
```

### 2단계: 대상 PC로 전송

- `PingMonitor_v2.6_Release.zip` 파일 복사

### 3단계: 압축 해제

```
C:\PingMonitor\          ✅ 추천
D:\Tools\PingMonitor\    ✅ 가능
C:\사용자\문서\...       ❌ 한글 경로 문제
```

### 4단계: 설정 파일 편집

```cmd
notepad config\ping_config.ini
```

### 5단계: 실행

```cmd
ping_monitor.exe
```

---

## 💡 팁

### 용량 최적화

- **Chart.js 제거 가능**: 오프라인 환경에서 차트 불필요 시
  - `web/chart.umd.min.js` 삭제 → 배포 크기 -205KB
  - 그래프 표시 안 됨, 나머지 기능은 정상 작동

### 보안

- **int_config.ini 절대 공유 금지**
  - 내부 IP 정보 포함
  - Git에 커밋하지 말 것
  - 배포 패키지에서 제외

### 백업

```cmd
# 데이터 백업
xcopy /E /I data data_backup_%DATE%

# 설정 백업
copy config\ping_config.ini config\ping_config.ini.%DATE%.bak
```

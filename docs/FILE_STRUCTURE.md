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

**포함 내용:**

- 프로젝트 디렉토리 구조
- 각 파일/폴더 설명
- 배포 패키지 구조
- 런타임 생성 파일
- .gitignore 규칙 설명

### 프로젝트 디렉토리

```
ping-monitor-webview/
│
├─ 소스 코드
│  ├─ ping_monitor_webview.c    # 메인 프로그램
│  ├─ http_server.c              # HTTP 서버 구현
│  └─ http_server.h              # HTTP 서버 헤더
│
├─ 빌드 스크립트
│  └─ build.bat                  # 컴파일 및 배포 스크립트
│
├─ 프론트엔드
│  ├─ graph.html                 # 대시보드 HTML
│  ├─ chart.umd.min.js           # Chart.js 라이브러리 (로컬)
│  └─ css/
│     ├─ variables.css           # CSS 변수 정의
│     ├─ base.css                # 기본 스타일
│     ├─ components.css          # 컴포넌트 스타일
│     ├─ dashboard.css           # 대시보드 레이아웃
│     ├─ notifications.css       # 알림 탭 스타일
│     └─ responsive.css          # 반응형 디자인
│
├─ 설정 파일
│  ├─ ping_config.ini            # 공개 IP 설정 (Git 추적)
│  └─ int_config.ini             # 내부 IP 설정 (Git 무시)
│
├─ 문서
│  ├─ README.md                  # 프로젝트 문서 (this file)
│  └─ DEPLOY_GUIDE.txt           # 배포 가이드
│
└─ 빌드 출력
   ├─ ping_monitor.exe           # 실행 파일
   ├─ ping_data.json             # 실시간 데이터 (자동 생성)
   └─ notification_log.json      # 알림 로그 (자동 생성)
```

### 배포 패키지 구조

```
PingMonitor/
├─ ping_monitor.exe
├─ graph.html
├─ chart.umd.min.js
├─ ping_config.ini
├─ int_config.ini (optional)
├─ css/
│  ├─ variables.css
│  ├─ base.css
│  ├─ components.css
│  ├─ dashboard.css
│  ├─ notifications.css
│  └─ responsive.css
└─ README.txt
```

---

## 📊 데이터 형식

### ping_data.json (자동 생성)

```json
{
  "running": true,
  "elapsed": 120,
  "total": 0,
  "loop": true,
  "targets": [
    {
      "ip": "8.8.8.8",
      "name": "Google DNS",
      "latency": 35,
      "online": 1,
      "total": 120,
      "success": 118,
      "min": 32,
      "max": 45,
      "avg": 35.2,
      "history": [35, 34, 36, 37, ...]
    }
  ]
}
```

### notification_log.json (자동 생성)

```json
[
  {
    "type": "timeout",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "date": "2026-01-28",
    "time": "14:32:15"
  },
  {
    "type": "recovery",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "date": "2026-01-28",
    "time": "14:33:20"
  }
]
```

---

## 📁 새로운 파일 : v2.6

### 소스 코드
```
outage.h                    - 장애 관리 헤더
outage.c                    - 장애 관리 구현
config_api.h                - 설정 API 헤더
config_api.c                - 설정 API 구현
browser_monitor.h           - 브라우저 모니터 헤더
browser_monitor.c           - 브라우저 모니터 구현
```

### CSS 파일
```
css/outages.css            - 장애 타임라인 스타일
css/settings.css           - 설정 UI 스타일
```

### 문서
```
BROWSER_CLOSE_DETECTION.md  - 브라우저 모니터링 완전 가이드
CHART_INSTALL.md            - Chart.js 설치 가이드
WHATS_NEW_v2.6.md           - v2.6 새 기능 요약
CHANGELOG_v2.6.md           - 이 문서
```

### 데이터 파일
```
outage_log.json            - 장애 로그 (자동 생성)
```
